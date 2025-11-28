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
    srcs = ["audio_node_graph.cc"],
    hdrs = ["audio_node_graph.h"],
    deps = [
        ":audio_interface",
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

exports_files([
    "audio.js",
    "audio_pre.js",
])
