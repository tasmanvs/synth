cc_library(
    name = "main_window",
    srcs = ["main_window.cc"],
    hdrs = ["main_window.h"],
    deps = [
        "@imgui//:imgui",
        "@implot//:implot",
        "//node_editor:imgui_node_editor",
        "//audio:audio_interface",
        "//audio:audio_node_graph",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
    ],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

cc_library(
    name = "implot_utils",
    srcs = ["implot_utils.cc"],
    hdrs = ["implot_utils.h"],
    deps = [
        "@implot//:implot",
    ],
    visibility = ["//visibility:public"],
)
