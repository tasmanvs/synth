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

exports_files([
    "audio.js",
    "audio_pre.js",
])
