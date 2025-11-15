#pragma once
// Stub ImGui Node Editor header for testing without UI dependencies
#ifndef IMGUI_NODE_EDITOR_H
#define IMGUI_NODE_EDITOR_H

#include "audio/imgui_stub.h"

namespace ax {
namespace NodeEditor {

struct Config { 
    const char* SettingsFile; 
};

struct EditorContext {};

struct PinId { 
    int id_; 
    PinId(int id = 0) : id_(id) {} 
    int Get() const { return id_; }
    operator bool() const { return id_ != 0; }
};

struct LinkId { 
    int id_; 
    LinkId(int id = 0) : id_(id) {} 
    int Get() const { return id_; }
};

struct NodeId { 
    int id_; 
    NodeId(int id = 0) : id_(id) {} 
    int Get() const { return id_; }
};

enum class PinKind { Input, Output };

inline EditorContext* CreateEditor(const Config*) { return nullptr; }
inline void DestroyEditor(EditorContext*) {}
inline void SetCurrentEditor(EditorContext*) {}
inline void Begin(const char*, const ImVec2&) {}
inline void End() {}
inline void BeginNode(int) {}
inline void EndNode() {}
inline void BeginPin(int, PinKind) {}
inline void EndPin() {}
inline void Link(int, int, int) {}
inline bool BeginCreate() { return false; }
inline void EndCreate() {}
inline bool QueryNewLink(PinId*, PinId*) { return false; }
inline bool AcceptNewItem() { return false; }
inline bool BeginDelete() { return false; }
inline void EndDelete() {}
inline bool QueryDeletedLink(LinkId*) { return false; }
inline bool QueryDeletedNode(NodeId*) { return false; }
inline bool AcceptDeletedItem() { return false; }
inline void Suspend() {}
inline void Resume() {}
inline bool ShowBackgroundContextMenu() { return false; }
inline void SetNodePosition(int, const ImVec2&) {}
inline void NavigateToContent(float) {}

} // namespace NodeEditor
} // namespace ax

#endif // IMGUI_NODE_EDITOR_H
