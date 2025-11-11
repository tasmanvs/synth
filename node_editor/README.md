# ImGui Node Editor

This directory contains the [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) library integrated into this Bazel project.

## About

Node Editor is an implementation of a node editor with ImGui-like API. It provides:
- Node placement, dragging, and selection
- Zoom and scrolling
- Fully customizable node and pin contents
- Bezier curve-based links
- Context menu support
- Shortcuts (cut/copy/paste/delete)

## Files

The library consists of the following source files:
- `imgui_node_editor.h` - Main header file
- `imgui_node_editor.cpp` - Main implementation
- `imgui_node_editor_api.cpp` - API implementation
- `imgui_node_editor_internal.h` - Internal header
- `imgui_node_editor_internal.inl` - Internal inline implementations
- `imgui_canvas.h/cpp` - Canvas implementation
- `imgui_bezier_math.h/inl` - Bezier curve math
- `imgui_extra_math.h/inl` - Extra math utilities
- `crude_json.h/cpp` - JSON serialization support

## Usage

To use the node editor in your Bazel target, add it as a dependency:

```starlark
cc_library(
    name = "my_app",
    srcs = ["my_app.cc"],
    deps = [
        "//node_editor:imgui_node_editor",
        "@imgui//:imgui",
    ],
)
```

In your code:

```cpp
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

// Create editor context
ed::Config config;
ed::EditorContext* context = ed::CreateEditor(&config);

// In your render loop
ed::SetCurrentEditor(context);
ed::Begin("My Editor");
// ... draw nodes and pins ...
ed::End();
ed::SetCurrentEditor(nullptr);

// Cleanup
ed::DestroyEditor(context);
```

## Modifications

The following modifications were made to ensure compatibility with ImGui 1.91.8:
- Replaced `ImGui::GetKeyIndex()` calls with direct `ImGuiKey` enum values (deprecated in newer ImGui versions)

## License

This library is licensed under the MIT License. See the `LICENSE` file for details.

## Original Repository

https://github.com/thedmd/imgui-node-editor
