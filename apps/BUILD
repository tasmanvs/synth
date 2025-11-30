cc_binary(
    name = "imgui_webgl_bin.html",
    srcs = ["imgui_webgl.cc"],
    additional_linker_inputs = [
        "//:shell.html",
        "//audio:audio_pre.js",
        "//audio:audio.js",
    ],
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
        "-sENVIRONMENT=web",
        # Enable exceptions for better error handling
        "-fexceptions",
        "-sDISABLE_EXCEPTION_CATCHING=0",
        # Optimize for size with compression in mind
        "-sMALLOC=emmalloc",
        # Provide better error messages
        "-sASSERTIONS=1",
        # Use custom shell file - this should generate HTML
        "--shell-file=$(location //:shell.html)",
        # Include pre-loaded JavaScript for Web Audio
        "--pre-js=$(location //audio:audio_pre.js)",
        # Include JavaScript library for Web Audio
        "--js-library=$(location //audio:audio.js)",
    ],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

# Alias for easier reference
alias(
    name = "imgui_webgl_bin",
    actual = ":imgui_webgl_bin.html",
    visibility = ["//visibility:public"],
)

# Extract the emscripten output tar archive and copy HTML
genrule(
    name = "imgui_webgl_html",
    srcs = [
        ":imgui_webgl_bin.html",
        "imgui_webgl.html",
    ],
    outs = [
        "imgui_webgl_bin.js",
        "imgui_webgl_bin.wasm",
        "imgui_webgl_index.html",
    ],
    cmd = """
        tar -xf $(location :imgui_webgl_bin.html) -C $$(dirname $(location imgui_webgl_bin.js))
        cp $(location imgui_webgl.html) $(location imgui_webgl_index.html)
    """,
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

# Create a filegroup that includes the generated files
filegroup(
    name = "imgui_webgl",
    srcs = [
        ":imgui_webgl_bin",
        ":imgui_webgl_html",
    ],
    output_group = "default",
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "imgui_webgl_wrapper",
    srcs = ["imgui_webgl_wrapper.cc"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "audio_webgl",
    srcs = ["imgui_webgl.cc"],
    deps = [
        "@imgui//:imgui",
        "//:imgui_glfw_opengl3_emscripten",
        "//ui:main_window",
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
