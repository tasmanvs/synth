cc_library(
    name = "audio_interface",
    srcs = ["audio_interface.cc"],
    hdrs = ["audio_interface.h"],
    deps = [
        "@abseil-cpp//absl/log",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "audio_node_graph",
    srcs = [
        "audio_node_graph.cc",
        "nodes/audio_node.cc",
        "nodes/source_node.cc",
        "nodes/frequency_source_node.cc",
        "nodes/frequency_offset_node.cc",
        "nodes/keyboard_frequency_node.cc",
        "nodes/harmonic_node.cc",
        "nodes/filter_nodes.cc",
        "nodes/effect_nodes.cc",
        "nodes/sum_node.cc",
        "nodes/player_node.cc",
    ],
    hdrs = [
        "audio_node_graph.h",
        "nodes/audio_node.h",
        "nodes/source_node.h",
        "nodes/frequency_source_node.h",
        "nodes/frequency_offset_node.h",
        "nodes/keyboard_frequency_node.h",
        "nodes/harmonic_node.h",
        "nodes/filter_nodes.h",
        "nodes/effect_nodes.h",
        "nodes/sum_node.h",
        "nodes/player_node.h",
    ],
    deps = [
        ":audio_interface",
        ":string_resonator_lib",
        "//node_editor:imgui_node_editor",
        "//audio_loop:audio_loop_lib",
        "@imgui//:imgui",
        "@implot//:implot",
        "@abseil-cpp//absl/log",
        "@pocketfft//:pocketfft",
    ],
    copts = ["-fexceptions"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "audio_node_graph_test",
    srcs = ["audio_node_graph_test.cc"],
    deps = [
        ":audio_node_graph",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "smoothed_square_test",
    srcs = ["smoothed_square_test.cc"],
    deps = [
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "string_resonator_lib",
    srcs = ["nodes/string_resonator.cc"],
    hdrs = ["nodes/string_resonator.h"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "string_resonator_standalone_test",
    srcs = ["nodes/string_resonator_standalone_test.cc"],
    deps = [":string_resonator_lib"],
)

exports_files([
    "audio.js",
    "audio_pre.js",
])
