cc_binary(
    name = "hello_world",
    srcs = ["hello_world.cc"],
    deps = [
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_binary(
    name = "imgui_hello",
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
)


