licenses(["notice"])  # Apache 2

# L7 HTTP filter that bridges HTTP/1.1 unary "gRPC" to compliant HTTP/2 gRPC.
# Public docs: docs/root/configuration/http_filters/grpc_http1_bridge_filter.rst

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "private_proto_test_lib",
    srcs = ["private_proto_test.cc"],
    hdrs = ["private_proto_test.h"],
    deps = [
        "//include/envoy/http:codes_interface",
        "//include/envoy/http:filter_interface",
        "//include/envoy/upstream:cluster_manager_interface",
        "//source/common/common:enum_to_int",
        "//source/common/common:utility_lib",
        "//source/common/http:headers_lib",
        "//source/common/http/http1:codec_lib",
        "//source/common/common:minimal_logger_lib",
    ],
)

envoy_cc_library(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        "//include/envoy/registry",
        "//include/envoy/server:filter_config_interface",
        "//source/extensions/filters/http:well_known_names",
        "//source/extensions/filters/http/private_proto_test:private_proto_test_lib",
    ],
)
