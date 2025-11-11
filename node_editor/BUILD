cc_library(
    name = "imgui_node_editor",
    srcs = [
        "imgui_node_editor.cpp",
        "imgui_node_editor_api.cpp",
        "imgui_canvas.cpp",
        "crude_json.cpp",
    ],
    hdrs = [
        "imgui_node_editor.h",
        "imgui_node_editor_internal.h",
        "imgui_node_editor_internal.inl",
        "imgui_canvas.h",
        "imgui_bezier_math.h",
        "imgui_bezier_math.inl",
        "imgui_extra_math.h",
        "imgui_extra_math.inl",
        "crude_json.h",
    ],
    deps = [
        "@imgui//:imgui",
    ],
    includes = ["."],
    copts = ["-Wno-unused-but-set-variable"],
    visibility = ["//visibility:public"],
    target_compatible_with = ["@platforms//cpu:wasm32"],
)
