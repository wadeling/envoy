#include <memory>

#include "envoy/api/v2/auth/cert.pb.h"
#include "envoy/common/exception.h"
#include "envoy/service/discovery/v2/sds.pb.h"

#include "common/secret/sds_api.h"
#include "common/ssl/certificate_validation_context_config_impl.h"
#include "common/ssl/tls_certificate_config_impl.h"

#include "test/mocks/grpc/mocks.h"
#include "test/mocks/init/mocks.h"
#include "test/mocks/secret/mocks.h"
#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace Envoy {
namespace Secret {
namespace {

class SdsApiTest : public testing::Test {
protected:
  SdsApiTest() : api_(Api::createApiForTest()) {}

  Api::ApiPtr api_;
};

// Validate that SdsApi object is created and initialized successfully.
TEST_F(SdsApiTest, BasicTest) {
  ::testing::InSequence s;
  const envoy::service::discovery::v2::SdsDummy dummy;
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  NiceMock<Init::ExpectableWatcherImpl> init_watcher;
  Init::TargetHandlePtr init_target_handle;
  EXPECT_CALL(init_manager, add(_))
      .WillOnce(Invoke([&init_target_handle](const Init::Target& target) {
        init_target_handle = target.createHandle("test");
      }));

  envoy::api::v2::core::ConfigSource config_source;
  config_source.mutable_api_config_source()->set_api_type(
      envoy::api::v2::core::ApiConfigSource::GRPC);
  auto grpc_service = config_source.mutable_api_config_source()->add_grpc_services();
  auto google_grpc = grpc_service->mutable_google_grpc();
  google_grpc->set_target_uri("fake_address");
  google_grpc->set_stat_prefix("test");
  TlsCertificateSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  Grpc::MockAsyncClient* grpc_client{new NiceMock<Grpc::MockAsyncClient>()};
  NiceMock<Grpc::MockAsyncClientFactory>* factory{new NiceMock<Grpc::MockAsyncClientFactory>()};
  EXPECT_CALL(server.cluster_manager_.async_client_manager_, factoryForGrpcService(_, _, _))
      .WillOnce(Invoke([factory](const envoy::api::v2::core::GrpcService&, Stats::Scope&, bool) {
        return Grpc::AsyncClientFactoryPtr{factory};
      }));
  EXPECT_CALL(*factory, create()).WillOnce(Invoke([grpc_client] {
    return Grpc::RawAsyncClientPtr{grpc_client};
  }));
  EXPECT_CALL(init_watcher, ready());
  init_target_handle->initialize(init_watcher);
}

// Validate that TlsCertificateSdsApi updates secrets successfully if a good secret
// is passed to onConfigUpdate().
TEST_F(SdsApiTest, DynamicTlsCertificateUpdateSuccess) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  TlsCertificateSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  NiceMock<Secret::MockSecretCallbacks> secret_callback;
  auto handle =
      sds_api.addUpdateCallback([&secret_callback]() { secret_callback.onAddOrUpdateSecret(); });

  std::string yaml =
      R"EOF(
  name: "abc.com"
  tls_certificate:
    certificate_chain:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem"
    private_key:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem"
    )EOF";
  envoy::api::v2::auth::Secret typed_secret;
  TestUtility::loadFromYaml(TestEnvironment::substitute(yaml), typed_secret);
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> secret_resources;
  secret_resources.Add()->PackFrom(typed_secret);

  EXPECT_CALL(secret_callback, onAddOrUpdateSecret());
  sds_api.onConfigUpdate(secret_resources, "");

  Ssl::TlsCertificateConfigImpl tls_config(*sds_api.secret(), *api_);
  const std::string cert_pem =
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem";
  EXPECT_EQ(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(cert_pem)),
            tls_config.certificateChain());

  const std::string key_pem =
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem";
  EXPECT_EQ(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(key_pem)),
            tls_config.privateKey());

  handle->remove();
}

class PartialMockSds : public SdsApi {
public:
  PartialMockSds(NiceMock<Server::MockInstance>& server, Api::Api& api,
                 NiceMock<Init::MockManager>& init_manager,
                 envoy::api::v2::core::ConfigSource& config_source)
      : SdsApi(
            server.localInfo(), server.dispatcher(), server.random(), server.stats(),
            server.clusterManager(), init_manager, config_source, "abc.com", []() {},
            server.messageValidationVisitor(), api) {}

  MOCK_METHOD2(onConfigUpdate,
               void(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>&, const std::string&));
  void onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>& added,
                      const Protobuf::RepeatedPtrField<std::string>& removed,
                      const std::string& version) override {
    SdsApi::onConfigUpdate(added, removed, version);
  }
  void setSecret(const envoy::api::v2::auth::Secret&) override {}
  void validateConfig(const envoy::api::v2::auth::Secret&) override {}
};

// Basic test of delta's passthrough call to the state-of-the-world variant, to
// increase coverage.
TEST_F(SdsApiTest, Delta) {
  Protobuf::RepeatedPtrField<envoy::api::v2::Resource> resources;
  envoy::api::v2::auth::Secret secret;
  secret.set_name("secret_1");
  auto* resource = resources.Add();
  resource->mutable_resource()->PackFrom(secret);
  resource->set_name("secret_1");
  resource->set_version("version1");

  Protobuf::RepeatedPtrField<ProtobufWkt::Any> for_matching;
  for_matching.Add()->PackFrom(secret);

  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  PartialMockSds sds(server, *api_, init_manager, config_source);
  EXPECT_CALL(sds, onConfigUpdate(RepeatedProtoEq(for_matching), "version1"));
  sds.SdsApi::onConfigUpdate(resources, {}, "ignored");

  // An attempt to remove a resource logs an error, but otherwise just carries on (ignoring the
  // removal attempt).
  resource->set_version("version2");
  EXPECT_CALL(sds, onConfigUpdate(RepeatedProtoEq(for_matching), "version2"));
  Protobuf::RepeatedPtrField<std::string> removals;
  *removals.Add() = "route_0";
  sds.SdsApi::onConfigUpdate(resources, removals, "ignored");
}

// Tests SDS's use of the delta variant of onConfigUpdate().
TEST_F(SdsApiTest, DeltaUpdateSuccess) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  TlsCertificateSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  NiceMock<Secret::MockSecretCallbacks> secret_callback;
  auto handle =
      sds_api.addUpdateCallback([&secret_callback]() { secret_callback.onAddOrUpdateSecret(); });

  std::string yaml =
      R"EOF(
  name: "abc.com"
  tls_certificate:
    certificate_chain:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem"
    private_key:
      filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem"
    )EOF";
  envoy::api::v2::auth::Secret typed_secret;
  TestUtility::loadFromYaml(TestEnvironment::substitute(yaml), typed_secret);
  Protobuf::RepeatedPtrField<envoy::api::v2::Resource> secret_resources;
  secret_resources.Add()->mutable_resource()->PackFrom(typed_secret);

  EXPECT_CALL(secret_callback, onAddOrUpdateSecret());
  sds_api.onConfigUpdate(secret_resources, {}, "");

  Ssl::TlsCertificateConfigImpl tls_config(*sds_api.secret(), *api_);
  const std::string cert_pem =
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem";
  EXPECT_EQ(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(cert_pem)),
            tls_config.certificateChain());

  const std::string key_pem =
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem";
  EXPECT_EQ(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(key_pem)),
            tls_config.privateKey());

  handle->remove();
}

// Validate that CertificateValidationContextSdsApi updates secrets successfully if
// a good secret is passed to onConfigUpdate().
TEST_F(SdsApiTest, DynamicCertificateValidationContextUpdateSuccess) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  CertificateValidationContextSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  NiceMock<Secret::MockSecretCallbacks> secret_callback;
  auto handle =
      sds_api.addUpdateCallback([&secret_callback]() { secret_callback.onAddOrUpdateSecret(); });

  std::string yaml =
      R"EOF(
  name: "abc.com"
  validation_context:
    trusted_ca: { filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem" }
    allow_expired_certificate: true
  )EOF";

  envoy::api::v2::auth::Secret typed_secret;
  TestUtility::loadFromYaml(TestEnvironment::substitute(yaml), typed_secret);
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> secret_resources;
  secret_resources.Add()->PackFrom(typed_secret);
  EXPECT_CALL(secret_callback, onAddOrUpdateSecret());
  sds_api.onConfigUpdate(secret_resources, "");

  Ssl::CertificateValidationContextConfigImpl cvc_config(*sds_api.secret(), *api_);
  const std::string ca_cert =
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem";
  EXPECT_EQ(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(ca_cert)),
            cvc_config.caCert());

  handle->remove();
}

class CvcValidationCallback {
public:
  virtual ~CvcValidationCallback() {}
  virtual void validateCvc(const envoy::api::v2::auth::CertificateValidationContext&) PURE;
};

class MockCvcValidationCallback : public CvcValidationCallback {
public:
  MockCvcValidationCallback() {}
  ~MockCvcValidationCallback() {}
  MOCK_METHOD1(validateCvc, void(const envoy::api::v2::auth::CertificateValidationContext&));
};

// Validate that CertificateValidationContextSdsApi updates secrets successfully if
// a good secret is passed to onConfigUpdate(), and that merged CertificateValidationContext
// provides correct information.
TEST_F(SdsApiTest, DefaultCertificateValidationContextTest) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  CertificateValidationContextSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  NiceMock<Secret::MockSecretCallbacks> secret_callback;
  auto handle =
      sds_api.addUpdateCallback([&secret_callback]() { secret_callback.onAddOrUpdateSecret(); });
  NiceMock<MockCvcValidationCallback> validation_callback;
  auto validation_handle = sds_api.addValidationCallback(
      [&validation_callback](const envoy::api::v2::auth::CertificateValidationContext& cvc) {
        validation_callback.validateCvc(cvc);
      });

  envoy::api::v2::auth::Secret typed_secret;
  typed_secret.set_name("abc.com");
  auto* dynamic_cvc = typed_secret.mutable_validation_context();
  dynamic_cvc->set_allow_expired_certificate(false);
  dynamic_cvc->mutable_trusted_ca()->set_filename(TestEnvironment::substitute(
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem"));
  dynamic_cvc->add_verify_subject_alt_name("second san");
  const std::string dynamic_verify_certificate_spki =
      "QGJRPdmx/r5EGOFLb2MTiZp2isyC0Whht7iazhzXaCM=";
  dynamic_cvc->add_verify_certificate_spki(dynamic_verify_certificate_spki);
  EXPECT_CALL(secret_callback, onAddOrUpdateSecret());
  EXPECT_CALL(validation_callback, validateCvc(_));

  Protobuf::RepeatedPtrField<ProtobufWkt::Any> secret_resources;
  secret_resources.Add()->PackFrom(typed_secret);
  sds_api.onConfigUpdate(secret_resources, "");

  const std::string default_verify_certificate_hash =
      "0000000000000000000000000000000000000000000000000000000000000000";
  envoy::api::v2::auth::CertificateValidationContext default_cvc;
  default_cvc.set_allow_expired_certificate(true);
  default_cvc.mutable_trusted_ca()->set_inline_bytes("fake trusted ca");
  default_cvc.add_verify_subject_alt_name("first san");
  default_cvc.add_verify_certificate_hash(default_verify_certificate_hash);
  envoy::api::v2::auth::CertificateValidationContext merged_cvc = default_cvc;
  merged_cvc.MergeFrom(*sds_api.secret());
  Ssl::CertificateValidationContextConfigImpl cvc_config(merged_cvc, *api_);
  // Verify that merging CertificateValidationContext applies logical OR to bool
  // field.
  EXPECT_TRUE(cvc_config.allowExpiredCertificate());
  // Verify that singular fields are overwritten.
  const std::string ca_cert =
      "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/ca_cert.pem";
  EXPECT_EQ(TestEnvironment::readFileToStringForTest(TestEnvironment::substitute(ca_cert)),
            cvc_config.caCert());
  // Verify that repeated fields are concatenated.
  EXPECT_EQ(2, cvc_config.verifySubjectAltNameList().size());
  EXPECT_EQ("first san", cvc_config.verifySubjectAltNameList()[0]);
  EXPECT_EQ("second san", cvc_config.verifySubjectAltNameList()[1]);
  // Verify that if dynamic CertificateValidationContext does not set certificate hash list, the new
  // secret contains hash list from default CertificateValidationContext.
  EXPECT_EQ(1, cvc_config.verifyCertificateHashList().size());
  EXPECT_EQ(default_verify_certificate_hash, cvc_config.verifyCertificateHashList()[0]);
  // Verify that if default CertificateValidationContext does not set certificate SPKI list, the new
  // secret contains SPKI list from dynamic CertificateValidationContext.
  EXPECT_EQ(1, cvc_config.verifyCertificateSpkiList().size());
  EXPECT_EQ(dynamic_verify_certificate_spki, cvc_config.verifyCertificateSpkiList()[0]);

  handle->remove();
  validation_handle->remove();
}

// Validate that SdsApi throws exception if an empty secret is passed to onConfigUpdate().
TEST_F(SdsApiTest, EmptyResource) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  TlsCertificateSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  Protobuf::RepeatedPtrField<ProtobufWkt::Any> secret_resources;

  EXPECT_THROW_WITH_MESSAGE(sds_api.onConfigUpdate(secret_resources, ""), EnvoyException,
                            "Missing SDS resources for abc.com in onConfigUpdate()");
}

// Validate that SdsApi throws exception if multiple secrets are passed to onConfigUpdate().
TEST_F(SdsApiTest, SecretUpdateWrongSize) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  TlsCertificateSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  std::string yaml =
      R"EOF(
    name: "abc.com"
    tls_certificate:
      certificate_chain:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem"
      private_key:
        filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem"
      )EOF";

  envoy::api::v2::auth::Secret typed_secret;
  TestUtility::loadFromYaml(TestEnvironment::substitute(yaml), typed_secret);
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> secret_resources;
  secret_resources.Add()->PackFrom(typed_secret);
  secret_resources.Add()->PackFrom(typed_secret);

  EXPECT_THROW_WITH_MESSAGE(sds_api.onConfigUpdate(secret_resources, ""), EnvoyException,
                            "Unexpected SDS secrets length: 2");
}

// Validate that SdsApi throws exception if secret name passed to onConfigUpdate()
// does not match configured name.
TEST_F(SdsApiTest, SecretUpdateWrongSecretName) {
  NiceMock<Server::MockInstance> server;
  NiceMock<Init::MockManager> init_manager;
  envoy::api::v2::core::ConfigSource config_source;
  TlsCertificateSdsApi sds_api(
      server.localInfo(), server.dispatcher(), server.random(), server.stats(),
      server.clusterManager(), init_manager, config_source, "abc.com", []() {},
      server.messageValidationVisitor(), *api_);

  std::string yaml =
      R"EOF(
      name: "wrong.name.com"
      tls_certificate:
        certificate_chain:
          filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_cert.pem"
        private_key:
          filename: "{{ test_rundir }}/test/extensions/transport_sockets/tls/test_data/selfsigned_key.pem"
        )EOF";

  envoy::api::v2::auth::Secret typed_secret;
  TestUtility::loadFromYaml(TestEnvironment::substitute(yaml), typed_secret);
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> secret_resources;
  secret_resources.Add()->PackFrom(typed_secret);

  EXPECT_THROW_WITH_MESSAGE(sds_api.onConfigUpdate(secret_resources, ""), EnvoyException,
                            "Unexpected SDS secret (expecting abc.com): wrong.name.com");
}

} // namespace
} // namespace Secret
} // namespace Envoy
