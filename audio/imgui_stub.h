#pragma once
// Stub ImGui header for testing without UI dependencies
#ifndef IMGUI_H
#define IMGUI_H

#define IMGUI_API

struct ImVec2 { 
    float x, y; 
    ImVec2(float _x = 0.0f, float _y = 0.0f) : x(_x), y(_y) {} 
};

struct ImVec4 { 
    float x, y, z, w; 
    ImVec4(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f) 
        : x(_x), y(_y), z(_z), w(_w) {} 
};

namespace ImGui {
    inline void PushID(int) {}
    inline void PopID() {}
    inline void Text(const char*, ...) {}
    inline void PushItemWidth(float) {}
    inline void PopItemWidth() {}
    inline bool SliderFloat(const char*, float*, float, float, const char* = nullptr) { return false; }
    inline bool Button(const char*) { return false; }
    inline void Separator() {}
    inline void OpenPopup(const char*) {}
    inline bool BeginPopup(const char*) { return false; }
    inline void EndPopup() {}
    inline bool MenuItem(const char*) { return false; }
}

#endif // IMGUI_H
