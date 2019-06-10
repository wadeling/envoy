licenses(["notice"])  # Apache 2

# QUICHE is Google's implementation of QUIC and related protocols. It is the
# same code used in Chromium and Google's servers, but packaged in a form that
# is intended to be easier to incorporate into third-party projects.
#
# QUICHE code falls into three groups:
# 1. Platform-independent code. Most QUICHE code is in this category.
# 2. APIs and type aliases to platform-dependent code/types, referenced by code
#    in group 1. This group is called the "Platform API".
# 3. Definitions of types declared in group 2. This group is called the
#    "Platform impl", and must be provided by the codebase that embeds QUICHE.
#
# Concretely, header files in group 2 (the Platform API) #include header and
# source files in group 3 (the Platform impl). Unfortunately, QUICHE does not
# yet provide a built-in way to customize this dependency, e.g. to override the
# directory or namespace in which Platform impl types are defined. Hence the
# gross hacks in quiche.genrule_cmd, invoked from here to tweak QUICHE source
# files into a form usable by Envoy.
#
# The mechanics of this will change as QUICHE evolves, supplies its own Bazel
# buildfiles, and provides a built-in way to override platform impl directory
# location. However, the end result (QUICHE files placed under
# quiche/{http2,quic,spdy}/, with the Envoy-specific implementation of the
# QUICHE platform APIs in //source/extensions/quic_listeners/quiche/platform/,
# should remain largely the same.

load(":genrule_cmd.bzl", "genrule_cmd")
load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_cc_test",
    "envoy_cc_test_library",
)

src_files = glob([
    "**/*.h",
    "**/*.c",
    "**/*.cc",
    "**/*.inc",
    "**/*.proto",
])

genrule(
    name = "quiche_files",
    srcs = src_files,
    outs = ["quiche/" + f for f in src_files],
    cmd = genrule_cmd("@envoy//bazel/external:quiche.genrule_cmd"),
    visibility = ["//visibility:private"],
)

# These options are only used to suppress errors in brought-in QUICHE tests.
# Use #pragma GCC diagnostic ignored in integration code to suppress these errors.
quiche_copt = [
    "-Wno-unused-parameter",
    # quic_inlined_frame.h uses offsetof() to optimize memory usage in frames.
    "-Wno-invalid-offsetof",
]

envoy_cc_test_library(
    name = "http2_platform_reconstruct_object",
    hdrs = ["quiche/http2/platform/api/http2_reconstruct_object.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:http2_platform_reconstruct_object_impl_lib"],
)

envoy_cc_test_library(
    name = "http2_test_tools_random",
    srcs = ["quiche/http2/test_tools/http2_random.cc"],
    hdrs = ["quiche/http2/test_tools/http2_random.h"],
    external_deps = ["ssl"],
    repository = "@envoy",
    deps = [":http2_platform"],
)

envoy_cc_library(
    name = "http2_platform",
    hdrs = [
        "quiche/http2/platform/api/http2_arraysize.h",
        "quiche/http2/platform/api/http2_bug_tracker.h",
        "quiche/http2/platform/api/http2_containers.h",
        "quiche/http2/platform/api/http2_estimate_memory_usage.h",
        "quiche/http2/platform/api/http2_export.h",
        "quiche/http2/platform/api/http2_flag_utils.h",
        "quiche/http2/platform/api/http2_flags.h",
        "quiche/http2/platform/api/http2_logging.h",
        "quiche/http2/platform/api/http2_macros.h",
        "quiche/http2/platform/api/http2_optional.h",
        "quiche/http2/platform/api/http2_ptr_util.h",
        "quiche/http2/platform/api/http2_string.h",
        "quiche/http2/platform/api/http2_string_piece.h",
        "quiche/http2/platform/api/http2_string_utils.h",
        # TODO: uncomment the following files as implementations are added.
        # "quiche/http2/platform/api/http2_test_helpers.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = ["@envoy//source/extensions/quic_listeners/quiche/platform:http2_platform_impl_lib"],
)

envoy_cc_library(
    name = "spdy_platform",
    hdrs = [
        "quiche/spdy/platform/api/spdy_arraysize.h",
        "quiche/spdy/platform/api/spdy_bug_tracker.h",
        "quiche/spdy/platform/api/spdy_containers.h",
        "quiche/spdy/platform/api/spdy_endianness_util.h",
        "quiche/spdy/platform/api/spdy_estimate_memory_usage.h",
        "quiche/spdy/platform/api/spdy_export.h",
        "quiche/spdy/platform/api/spdy_flags.h",
        "quiche/spdy/platform/api/spdy_logging.h",
        "quiche/spdy/platform/api/spdy_macros.h",
        "quiche/spdy/platform/api/spdy_mem_slice.h",
        "quiche/spdy/platform/api/spdy_ptr_util.h",
        "quiche/spdy/platform/api/spdy_string.h",
        "quiche/spdy/platform/api/spdy_string_piece.h",
        "quiche/spdy/platform/api/spdy_string_utils.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quiche_common_lib",
        "@envoy//source/extensions/quic_listeners/quiche/platform:spdy_platform_impl_lib",
    ],
)

envoy_cc_library(
    name = "spdy_simple_arena_lib",
    srcs = ["quiche/spdy/core/spdy_simple_arena.cc"],
    hdrs = ["quiche/spdy/core/spdy_simple_arena.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":spdy_platform"],
)

envoy_cc_test_library(
    name = "spdy_platform_test_helpers",
    hdrs = ["quiche/spdy/platform/api/spdy_test_helpers.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:spdy_platform_test_helpers_impl_lib"],
)

envoy_cc_library(
    name = "spdy_platform_unsafe_arena_lib",
    hdrs = ["quiche/spdy/platform/api/spdy_unsafe_arena.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = ["@envoy//source/extensions/quic_listeners/quiche/platform:spdy_platform_unsafe_arena_impl_lib"],
)

envoy_cc_library(
    name = "spdy_core_alt_svc_wire_format_lib",
    srcs = ["quiche/spdy/core/spdy_alt_svc_wire_format.cc"],
    hdrs = ["quiche/spdy/core/spdy_alt_svc_wire_format.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":spdy_platform"],
)

envoy_cc_library(
    name = "spdy_core_header_block_lib",
    srcs = ["quiche/spdy/core/spdy_header_block.cc"],
    hdrs = ["quiche/spdy/core/spdy_header_block.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":spdy_platform",
        ":spdy_platform_unsafe_arena_lib",
    ],
)

envoy_cc_library(
    name = "spdy_core_headers_handler_interface",
    hdrs = ["quiche/spdy/core/spdy_headers_handler_interface.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":spdy_platform"],
)

envoy_cc_library(
    name = "spdy_core_protocol_lib",
    hdrs = [
        "quiche/spdy/core/spdy_bitmasks.h",
        "quiche/spdy/core/spdy_protocol.h",
    ],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":spdy_core_alt_svc_wire_format_lib",
        ":spdy_core_header_block_lib",
        ":spdy_platform",
    ],
)

envoy_cc_test_library(
    name = "spdy_core_test_utils_lib",
    srcs = ["quiche/spdy/core/spdy_test_utils.cc"],
    hdrs = ["quiche/spdy/core/spdy_test_utils.h"],
    copts = quiche_copt,
    repository = "@envoy",
    deps = [
        ":spdy_core_header_block_lib",
        ":spdy_core_headers_handler_interface",
        ":spdy_core_protocol_lib",
        ":spdy_platform",
    ],
)

envoy_cc_library(
    name = "quic_platform",
    srcs = [
        "quiche/quic/platform/api/quic_clock.cc",
        "quiche/quic/platform/api/quic_file_utils.cc",
        "quiche/quic/platform/api/quic_hostname_utils.cc",
        "quiche/quic/platform/api/quic_mutex.cc",
    ],
    hdrs = [
        "quiche/quic/platform/api/quic_cert_utils.h",
        "quiche/quic/platform/api/quic_clock.h",
        "quiche/quic/platform/api/quic_file_utils.h",
        "quiche/quic/platform/api/quic_hostname_utils.h",
        "quiche/quic/platform/api/quic_mutex.h",
        "quiche/quic/platform/api/quic_pcc_sender.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_time_lib",
        ":quic_platform_base",
        "@envoy//source/extensions/quic_listeners/quiche/platform:quic_platform_impl_lib",
    ],
)

envoy_cc_test_library(
    name = "quic_platform_epoll_lib",
    hdrs = ["quiche/quic/platform/api/quic_epoll.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_epoll_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_expect_bug",
    hdrs = ["quiche/quic/platform/api/quic_expect_bug.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_expect_bug_impl_lib"],
)

envoy_cc_library(
    name = "quic_platform_export",
    hdrs = ["quiche/quic/platform/api/quic_export.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = ["@envoy//source/extensions/quic_listeners/quiche/platform:quic_platform_export_impl_lib"],
)

envoy_cc_library(
    name = "quic_platform_ip_address_family",
    hdrs = ["quiche/quic/platform/api/quic_ip_address_family.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
)

envoy_cc_test_library(
    name = "quic_platform_mock_log",
    hdrs = ["quiche/quic/platform/api/quic_mock_log.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_mock_log_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_port_utils",
    hdrs = ["quiche/quic/platform/api/quic_port_utils.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_port_utils_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_sleep",
    hdrs = ["quiche/quic/platform/api/quic_sleep.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_sleep_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_test",
    hdrs = ["quiche/quic/platform/api/quic_test.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_test_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_test_output",
    hdrs = ["quiche/quic/platform/api/quic_test_output.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_test_output_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_system_event_loop",
    hdrs = ["quiche/quic/platform/api/quic_system_event_loop.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_system_event_loop_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_thread",
    hdrs = ["quiche/quic/platform/api/quic_thread.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_thread_impl_lib"],
)

envoy_cc_library(
    name = "quic_platform_base",
    srcs = [
        "quiche/quic/platform/api/quic_ip_address.cc",
        "quiche/quic/platform/api/quic_socket_address.cc",
    ],
    hdrs = [
        "quiche/quic/platform/api/quic_aligned.h",
        "quiche/quic/platform/api/quic_arraysize.h",
        "quiche/quic/platform/api/quic_bug_tracker.h",
        "quiche/quic/platform/api/quic_client_stats.h",
        "quiche/quic/platform/api/quic_containers.h",
        "quiche/quic/platform/api/quic_endian.h",
        "quiche/quic/platform/api/quic_estimate_memory_usage.h",
        "quiche/quic/platform/api/quic_exported_stats.h",
        "quiche/quic/platform/api/quic_fallthrough.h",
        "quiche/quic/platform/api/quic_flag_utils.h",
        "quiche/quic/platform/api/quic_flags.h",
        "quiche/quic/platform/api/quic_iovec.h",
        "quiche/quic/platform/api/quic_ip_address.h",
        "quiche/quic/platform/api/quic_logging.h",
        "quiche/quic/platform/api/quic_map_util.h",
        "quiche/quic/platform/api/quic_mem_slice.h",
        "quiche/quic/platform/api/quic_prefetch.h",
        "quiche/quic/platform/api/quic_ptr_util.h",
        "quiche/quic/platform/api/quic_reference_counted.h",
        "quiche/quic/platform/api/quic_server_stats.h",
        "quiche/quic/platform/api/quic_socket_address.h",
        "quiche/quic/platform/api/quic_stack_trace.h",
        "quiche/quic/platform/api/quic_str_cat.h",
        "quiche/quic/platform/api/quic_stream_buffer_allocator.h",
        "quiche/quic/platform/api/quic_string_piece.h",
        "quiche/quic/platform/api/quic_string_utils.h",
        "quiche/quic/platform/api/quic_uint128.h",
        "quiche/quic/platform/api/quic_text_utils.h",
        # TODO: uncomment the following files as implementations are added.
        # "quiche/quic/platform/api/quic_fuzzed_data_provider.h",
        # "quiche/quic/platform/api/quic_test_loopback.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_platform_export",
        ":quiche_common_lib",
        "@envoy//source/extensions/quic_listeners/quiche/platform:quic_platform_base_impl_lib",
        "@envoy//source/extensions/quic_listeners/quiche/platform:quic_platform_logging_impl_lib",
    ],
)

envoy_cc_library(
    name = "quic_core_buffer_allocator_lib",
    srcs = [
        "quiche/quic/core/quic_buffer_allocator.cc",
        "quiche/quic/core/quic_simple_buffer_allocator.cc",
    ],
    hdrs = [
        "quiche/quic/core/quic_buffer_allocator.h",
        "quiche/quic/core/quic_simple_buffer_allocator.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":quic_platform_export"],
)

envoy_cc_library(
    name = "quic_core_error_codes_lib",
    srcs = ["quiche/quic/core/quic_error_codes.cc"],
    hdrs = ["quiche/quic/core/quic_error_codes.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":quic_platform_export"],
)

envoy_cc_library(
    name = "quic_core_time_lib",
    srcs = ["quiche/quic/core/quic_time.cc"],
    hdrs = ["quiche/quic/core/quic_time.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":quic_platform_base"],
)

envoy_cc_library(
    name = "quic_core_types_lib",
    srcs = [
        "quiche/quic/core/quic_connection_id.cc",
        "quiche/quic/core/quic_packet_number.cc",
        "quiche/quic/core/quic_types.cc",
    ],
    hdrs = [
        "quiche/quic/core/quic_connection_id.h",
        "quiche/quic/core/quic_packet_number.h",
        "quiche/quic/core/quic_types.h",
    ],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_error_codes_lib",
        ":quic_core_time_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_crypto_random_lib",
    srcs = ["quiche/quic/core/crypto/quic_random.cc"],
    hdrs = ["quiche/quic/core/crypto/quic_random.h"],
    copts = quiche_copt,
    external_deps = ["ssl"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":quic_platform_base"],
)

envoy_cc_library(
    name = "quic_core_constants_lib",
    srcs = ["quiche/quic/core/quic_constants.cc"],
    hdrs = ["quiche/quic/core/quic_constants.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_types_lib",
        ":quic_platform_export",
    ],
)

envoy_cc_library(
    name = "quic_core_tag_lib",
    srcs = ["quiche/quic/core/quic_tag.cc"],
    hdrs = ["quiche/quic/core/quic_tag.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":quic_platform_base"],
)

envoy_cc_library(
    name = "quic_core_versions_lib",
    srcs = ["quiche/quic/core/quic_versions.cc"],
    hdrs = ["quiche/quic/core/quic_versions.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_tag_lib",
        ":quic_core_types_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_interval_lib",
    hdrs = ["quiche/quic/core/quic_interval.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
)

envoy_cc_library(
    name = "quic_core_interval_set_lib",
    hdrs = ["quiche/quic/core/quic_interval_set.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_interval_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_frames_frames_lib",
    srcs = [
        "quiche/quic/core/frames/quic_ack_frame.cc",
        "quiche/quic/core/frames/quic_blocked_frame.cc",
        "quiche/quic/core/frames/quic_connection_close_frame.cc",
        "quiche/quic/core/frames/quic_crypto_frame.cc",
        "quiche/quic/core/frames/quic_frame.cc",
        "quiche/quic/core/frames/quic_goaway_frame.cc",
        "quiche/quic/core/frames/quic_max_streams_frame.cc",
        "quiche/quic/core/frames/quic_message_frame.cc",
        "quiche/quic/core/frames/quic_new_connection_id_frame.cc",
        "quiche/quic/core/frames/quic_new_token_frame.cc",
        "quiche/quic/core/frames/quic_padding_frame.cc",
        "quiche/quic/core/frames/quic_path_challenge_frame.cc",
        "quiche/quic/core/frames/quic_path_response_frame.cc",
        "quiche/quic/core/frames/quic_ping_frame.cc",
        "quiche/quic/core/frames/quic_retire_connection_id_frame.cc",
        "quiche/quic/core/frames/quic_rst_stream_frame.cc",
        "quiche/quic/core/frames/quic_stop_sending_frame.cc",
        "quiche/quic/core/frames/quic_stop_waiting_frame.cc",
        "quiche/quic/core/frames/quic_stream_frame.cc",
        "quiche/quic/core/frames/quic_streams_blocked_frame.cc",
        "quiche/quic/core/frames/quic_window_update_frame.cc",
    ],
    hdrs = [
        "quiche/quic/core/frames/quic_ack_frame.h",
        "quiche/quic/core/frames/quic_blocked_frame.h",
        "quiche/quic/core/frames/quic_connection_close_frame.h",
        "quiche/quic/core/frames/quic_crypto_frame.h",
        "quiche/quic/core/frames/quic_frame.h",
        "quiche/quic/core/frames/quic_goaway_frame.h",
        "quiche/quic/core/frames/quic_inlined_frame.h",
        "quiche/quic/core/frames/quic_max_streams_frame.h",
        "quiche/quic/core/frames/quic_message_frame.h",
        "quiche/quic/core/frames/quic_mtu_discovery_frame.h",
        "quiche/quic/core/frames/quic_new_connection_id_frame.h",
        "quiche/quic/core/frames/quic_new_token_frame.h",
        "quiche/quic/core/frames/quic_padding_frame.h",
        "quiche/quic/core/frames/quic_path_challenge_frame.h",
        "quiche/quic/core/frames/quic_path_response_frame.h",
        "quiche/quic/core/frames/quic_ping_frame.h",
        "quiche/quic/core/frames/quic_retire_connection_id_frame.h",
        "quiche/quic/core/frames/quic_rst_stream_frame.h",
        "quiche/quic/core/frames/quic_stop_sending_frame.h",
        "quiche/quic/core/frames/quic_stop_waiting_frame.h",
        "quiche/quic/core/frames/quic_stream_frame.h",
        "quiche/quic/core/frames/quic_streams_blocked_frame.h",
        "quiche/quic/core/frames/quic_window_update_frame.h",
    ],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_buffer_allocator_lib",
        ":quic_core_constants_lib",
        ":quic_core_error_codes_lib",
        ":quic_core_interval_lib",
        ":quic_core_types_lib",
        ":quic_platform_base",
        ":quic_platform_mem_slice_span_lib",
    ],
)

envoy_cc_library(
    name = "quic_core_ack_listener_interface",
    srcs = ["quiche/quic/core/quic_ack_listener_interface.cc"],
    hdrs = ["quiche/quic/core/quic_ack_listener_interface.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_time_lib",
        ":quic_core_types_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_bandwidth_lib",
    srcs = ["quiche/quic/core/quic_bandwidth.cc"],
    hdrs = ["quiche/quic/core/quic_bandwidth.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_constants_lib",
        ":quic_core_time_lib",
        ":quic_core_types_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_data_lib",
    srcs = [
        # "quiche/quic/core/quic_data_reader.cc",
        "quiche/quic/core/quic_data_writer.cc",
    ],
    hdrs = [
        # "quiche/quic/core/quic_data_reader.h",
        "quiche/quic/core/quic_data_writer.h",
    ],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_constants_lib",
        ":quic_core_crypto_random_lib",
        ":quic_core_types_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_utils_lib",
    srcs = ["quiche/quic/core/quic_utils.cc"],
    hdrs = ["quiche/quic/core/quic_utils.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_constants_lib",
        ":quic_core_crypto_random_lib",
        ":quic_core_error_codes_lib",
        ":quic_core_frames_frames_lib",
        ":quic_core_types_lib",
        ":quic_core_versions_lib",
        ":quic_platform_base",
    ],
)

envoy_cc_library(
    name = "quic_core_stream_send_buffer_lib",
    srcs = ["quiche/quic/core/quic_stream_send_buffer.cc"],
    hdrs = ["quiche/quic/core/quic_stream_send_buffer.h"],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [
        ":quic_core_data_lib",
        ":quic_core_frames_frames_lib",
        ":quic_core_interval_lib",
        ":quic_core_interval_set_lib",
        ":quic_core_types_lib",
        ":quic_core_utils_lib",
        ":quic_platform_base",
        ":quic_platform_mem_slice_span_lib",
    ],
)

envoy_cc_test_library(
    name = "epoll_server_platform",
    hdrs = [
        "quiche/epoll_server/platform/api/epoll_address_test_utils.h",
        "quiche/epoll_server/platform/api/epoll_bug.h",
        "quiche/epoll_server/platform/api/epoll_expect_bug.h",
        "quiche/epoll_server/platform/api/epoll_export.h",
        "quiche/epoll_server/platform/api/epoll_logging.h",
        "quiche/epoll_server/platform/api/epoll_ptr_util.h",
        "quiche/epoll_server/platform/api/epoll_test.h",
        "quiche/epoll_server/platform/api/epoll_thread.h",
        "quiche/epoll_server/platform/api/epoll_time.h",
    ],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:epoll_server_platform_impl_lib"],
)

envoy_cc_test_library(
    name = "epoll_server_lib",
    srcs = select({
        "@envoy//bazel:linux": [
            "quiche/epoll_server/fake_simple_epoll_server.cc",
            "quiche/epoll_server/simple_epoll_server.cc",
        ],
        "//conditions:default": [],
    }),
    hdrs = select({
        "@envoy//bazel:linux": [
            "quiche/epoll_server/fake_simple_epoll_server.h",
            "quiche/epoll_server/simple_epoll_server.h",
        ],
        "//conditions:default": [],
    }),
    copts = quiche_copt,
    repository = "@envoy",
    deps = [":epoll_server_platform"],
)

envoy_cc_library(
    name = "quiche_common_platform",
    hdrs = [
        "quiche/common/platform/api/quiche_logging.h",
        "quiche/common/platform/api/quiche_ptr_util.h",
        "quiche/common/platform/api/quiche_unordered_containers.h",
    ],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = ["@envoy//source/extensions/quic_listeners/quiche/platform:quiche_common_platform_impl_lib"],
)

envoy_cc_test_library(
    name = "quiche_common_platform_test",
    hdrs = ["quiche/common/platform/api/quiche_test.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quiche_common_platform_test_impl_lib"],
)

envoy_cc_library(
    name = "quiche_common_lib",
    hdrs = ["quiche/common/simple_linked_hash_map.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = [":quiche_common_platform"],
)

envoy_cc_test(
    name = "epoll_server_test",
    srcs = select({
        "@envoy//bazel:linux": ["quiche/epoll_server/simple_epoll_server_test.cc"],
        "//conditions:default": [],
    }),
    copts = quiche_copt,
    repository = "@envoy",
    deps = [":epoll_server_lib"],
)

envoy_cc_test(
    name = "quiche_common_test",
    srcs = ["quiche/common/simple_linked_hash_map_test.cc"],
    copts = quiche_copt,
    repository = "@envoy",
    deps = [
        ":quiche_common_lib",
        ":quiche_common_platform_test",
    ],
)

envoy_cc_test(
    name = "http2_platform_api_test",
    srcs = [
        "quiche/http2/platform/api/http2_string_utils_test.cc",
        "quiche/http2/test_tools/http2_random_test.cc",
    ],
    repository = "@envoy",
    deps = [
        ":http2_platform",
        ":http2_test_tools_random",
    ],
)

envoy_cc_test(
    name = "spdy_platform_api_test",
    srcs = ["quiche/spdy/platform/api/spdy_string_utils_test.cc"],
    repository = "@envoy",
    deps = [":spdy_platform"],
)

envoy_cc_library(
    name = "quic_platform_mem_slice_span_lib",
    hdrs = [
        "quiche/quic/platform/api/quic_mem_slice_span.h",
    ],
    copts = quiche_copt,
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = ["@envoy//source/extensions/quic_listeners/quiche/platform:quic_platform_mem_slice_span_impl_lib"],
)

envoy_cc_test_library(
    name = "quic_platform_test_mem_slice_vector_lib",
    hdrs = ["quiche/quic/platform/api/quic_test_mem_slice_vector.h"],
    repository = "@envoy",
    deps = ["@envoy//test/extensions/quic_listeners/quiche/platform:quic_platform_test_mem_slice_vector_impl_lib"],
)

envoy_cc_library(
    name = "quic_platform_mem_slice_storage_lib",
    hdrs = ["quiche/quic/platform/api/quic_mem_slice_storage.h"],
    repository = "@envoy",
    visibility = ["//visibility:public"],
    deps = ["@envoy//source/extensions/quic_listeners/quiche/platform:quic_platform_mem_slice_storage_impl_lib"],
)

envoy_cc_test(
    name = "spdy_core_header_block_test",
    srcs = ["quiche/spdy/core/spdy_header_block_test.cc"],
    copts = quiche_copt,
    repository = "@envoy",
    deps = [
        ":spdy_core_header_block_lib",
        ":spdy_core_test_utils_lib",
    ],
)

envoy_cc_test(
    name = "quic_platform_api_test",
    srcs = [
        "quiche/quic/platform/api/quic_containers_test.cc",
        "quiche/quic/platform/api/quic_endian_test.cc",
        "quiche/quic/platform/api/quic_mem_slice_span_test.cc",
        "quiche/quic/platform/api/quic_mem_slice_storage_test.cc",
        "quiche/quic/platform/api/quic_mem_slice_test.cc",
        "quiche/quic/platform/api/quic_reference_counted_test.cc",
        "quiche/quic/platform/api/quic_string_utils_test.cc",
        "quiche/quic/platform/api/quic_text_utils_test.cc",
    ],
    copts = quiche_copt,
    repository = "@envoy",
    deps = [
        ":quic_core_buffer_allocator_lib",
        ":quic_platform",
        ":quic_platform_mem_slice_span_lib",
        ":quic_platform_mem_slice_storage_lib",
        ":quic_platform_test",
        ":quic_platform_test_mem_slice_vector_lib",
    ],
)
