licenses(["notice"])  # Apache 2

# Echo L4 network filter. This is primarily a simplistic example.
# Public docs: docs/root/configuration/network_filters/echo_filter.rst

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "echo2",
    srcs = ["echo2.cc"],
    hdrs = ["echo2.h"],
    external_deps = ["http_parser"],
    deps = [
        "//include/envoy/buffer:buffer_interface",
        "//include/envoy/network:connection_interface",
        "//include/envoy/network:filter_interface",
        "//source/common/common:assert_lib",
        "//source/common/common:minimal_logger_lib",
    ],
)

envoy_cc_library(
    name = "config",
    srcs = ["config.cc"],
    deps = [
        ":echo2",
        "//include/envoy/registry",
        "//include/envoy/server:filter_config_interface",
        "//source/extensions/filters/network:well_known_names",
        "//source/common/common:minimal_logger_lib",
    ],
)
