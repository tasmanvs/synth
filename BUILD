# Shared library for ImGui backends (used by web apps)
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
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)

# Export shell.html for web applications
exports_files([
    "shell.html",
])

# Custom compile commands target for clangd
# This only includes WebAssembly-compatible targets
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",
    targets = {
        "//apps:imgui_webgl_bin": "--platforms=@emsdk//:platform_wasm",
        "//ui:main_window": "--platforms=@emsdk//:platform_wasm",
        "//audio:audio_interface": "--platforms=@emsdk//:platform_wasm",
        "//audio:audio_node_graph": "--platforms=@emsdk//:platform_wasm",
        "//audio_loop:audio_loop_lib": "",
        "//node_editor:imgui_node_editor": "--platforms=@emsdk//:platform_wasm",
    },
)


