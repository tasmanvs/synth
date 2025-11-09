cc_binary(
    name = "imgui_webgl",
    srcs = ["imgui_webgl.cc"],
    deps = [
        "@imgui//:imgui",
        "//:imgui_glfw_opengl3_emscripten",
        "//ui:main_window",
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
        "--shell-file=$(location //:shell.html)",
        # Include pre-loaded JavaScript for Web Audio
        "--pre-js=$(location //audio:audio_pre.js)",
        # Include JavaScript library for Web Audio
        "--js-library=$(location //audio:audio.js)",
    ],
    data = [
        "//:shell.html",
        "//audio:audio_pre.js",
        "//audio:audio.js",
    ],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

cc_binary(
    name = "audio_webgl",
    srcs = ["imgui_webgl.cc"],
    deps = [
        "@imgui//:imgui",
        "//:imgui_glfw_opengl3_emscripten",
        "//ui:main_window",
        "//audio:audio_synth",
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
        "--shell-file=$(location //:shell.html)",
    ],
    data = [
        "//:shell.html",
    ],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)
