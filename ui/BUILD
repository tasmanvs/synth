cc_library(
    name = "main_window",
    srcs = ["main_window.cc"],
    hdrs = ["main_window.h"],
    deps = [
        "@imgui//:imgui",
        "@implot//:implot",
        "//audio:audio_synth",
        "//audio:audio_interface",
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
