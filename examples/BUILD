cc_binary(
    name = "hello_world",
    srcs = ["hello_world.cc"],
    deps = [
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "imgui_hello",
    srcs = ["imgui_hello.cc"],
    deps = [
        "@imgui//:imgui",
        "//:imgui_glfw_opengl3_emscripten",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "imgui_dx11",
    srcs = ["imgui_dx11.cc"],
    deps = [
        "@imgui//:imgui",
        "@imgui//backends:platform-win32",
        "@imgui//backends:renderer-dx11",
    ],
    linkopts = [
        "d3d11.lib",
        "dxgi.lib",
        "d3dcompiler.lib",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "flac_test",
    srcs = ["flac_test.cc"],
    deps = [
        "@flac//:flac",
    ],
    visibility = ["//visibility:public"],
)
