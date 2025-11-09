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

cc_library(
    name = "imgui_glfw_opengl3_emscripten",
    srcs = [
        "backends/imgui_impl_glfw.cpp",
        "backends/imgui_impl_opengl3.cpp",
    ],
    hdrs = [
        "backends/imgui_impl_glfw.h",
        "backends/imgui_impl_opengl3.h",
        "backends/imgui_impl_opengl3_loader.h",
    ],
    deps = [
        "@imgui//:imgui",
    ],
    defines = [
        "IMGUI_IMPL_OPENGL_ES3",
    ],
    strip_include_prefix = "backends",
)

cc_library(
    name = "main_window",
    srcs = ["main_window.cc"],
    hdrs = ["main_window.h"],
    deps = [
        "@imgui//:imgui",
        "@implot//:implot",
        ":audio_synth",
        ":audio_interface",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
    ],
)

cc_binary(
    name = "imgui_webgl",
    srcs = ["imgui_webgl.cc"],
    deps = [
        "@imgui//:imgui",
        ":imgui_glfw_opengl3_emscripten",
        ":main_window",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
    ],
    linkopts = [
        "-sUSE_GLFW=3",
        "-sUSE_WEBGL2=1",
        "-sALLOW_MEMORY_GROWTH=1",
        "-sFULL_ES3=1",
        "-sWASM=1",
        # Enable exceptions for better error handling
        "-fexceptions",
        "-sDISABLE_EXCEPTION_CATCHING=0",
        # Optimize for size with compression in mind
        "-sMALLOC=emmalloc",
        # Provide better error messages
        "-sASSERTIONS=1",
        # Use custom shell file
        "--shell-file=$(location :shell.html)",
        # Include pre-loaded JavaScript for Web Audio
        "--pre-js=$(location :audio_pre.js)",
        # Include JavaScript library for Web Audio
        "--js-library=$(location :audio.js)",
    ],
    data = [
        ":shell.html",
        ":audio_pre.js",
        ":audio.js",
    ],
)

cc_binary(
    name = "flac_test",
    srcs = ["flac_test.cc"],
    deps = [
        "@flac//:flac",
    ],
)

cc_library(
    name = "audio_synth",
    srcs = ["audio_synth.cc"],
    hdrs = ["audio_synth.h"],
    deps = [
        "@abseil-cpp//absl/log",
    ],
)

cc_library(
    name = "audio_interface",
    srcs = ["audio_interface.cc"],
    hdrs = ["audio_interface.h"],
    deps = [
        "@abseil-cpp//absl/log",
    ],
)

cc_binary(
    name = "audio_webgl",
    srcs = ["imgui_webgl.cc"],
    deps = [
        "@imgui//:imgui",
        ":imgui_glfw_opengl3_emscripten",
        ":main_window",
        ":audio_synth",
    ],
    linkopts = [
        "-sUSE_GLFW=3",
        "-sUSE_WEBGL2=1",
        "-sALLOW_MEMORY_GROWTH=1",
        "-sFULL_ES3=1",
        "-sWASM=1",
        # OpenAL support (mandatory for proper OpenAL usage)
        "-lopenal",
        # Enable exceptions for better error handling
        "-fexceptions",
        "-sDISABLE_EXCEPTION_CATCHING=0",
        # Optimize for size with compression in mind
        "-sMALLOC=emmalloc",
        # Provide better error messages
        "-sASSERTIONS=1",
        # Use custom shell file
        "--shell-file=$(location :shell.html)",
    ],
    data = [
        ":shell.html",
    ],
)



