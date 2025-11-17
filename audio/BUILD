cc_library(
    name = "audio_synth",
    srcs = ["audio_synth.cc"],
    hdrs = ["audio_synth.h"],
    deps = [
        "@abseil-cpp//absl/log",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "audio_interface",
    srcs = ["audio_interface.cc"],
    hdrs = ["audio_interface.h"],
    deps = [
        "@abseil-cpp//absl/log",
    ],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

cc_library(
    name = "audio_node_graph",
    srcs = ["audio_node_graph.cc"],
    hdrs = ["audio_node_graph.h"],
    deps = [
        ":audio_synth",
        ":audio_interface",
        "//node_editor:imgui_node_editor",
        "//audio_loop:audio_loop_lib",
        "@imgui//:imgui",
        "@implot//:implot",
        "@abseil-cpp//absl/log",
    ],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

exports_files([
    "audio.js",
    "audio_pre.js",
])
