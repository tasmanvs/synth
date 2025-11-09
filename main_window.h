#pragma once

#include "imgui.h"

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Update();
    void Draw();

private:
    bool show_demo_window_;
    bool show_another_window_;
    ImVec4 clear_color_;
    float slider_value_;
    int counter_;
};
