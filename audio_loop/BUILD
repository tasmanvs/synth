package(default_visibility = ["//visibility:public"])

cc_library(
    name = "audio_loop_lib",
    srcs = ["sine_buffer_generator.cc"],
    hdrs = ["sine_buffer_generator.h"],
)

cc_library(
    name = "audio_output",
    srcs = ["audio_output.cc"],
    hdrs = ["audio_output.h"],
    deps = [
        "@abseil-cpp//absl/log",
    ],
    linkopts = [
        "xaudio2.lib",
    ],
)

cc_binary(
    name = "buffer_visualizer",
    srcs = ["buffer_visualizer.cc"],
    deps = [
        ":audio_loop_lib",
        ":audio_output",
        "@imgui//:imgui",
        "@imgui//backends:platform-win32",
        "@imgui//backends:renderer-dx11",
        "@implot//:implot",
    ],
    linkopts = [
        "d3d11.lib",
        "dxgi.lib",
        "d3dcompiler.lib",
    ],
    target_compatible_with = ["@platforms//os:windows"],
)

cc_test(
    name = "audio_buffer_test",
    srcs = ["audio_buffer_test.cc"],
    deps = [
        ":audio_loop_lib",
        "@googletest//:gtest_main",
    ],
)
