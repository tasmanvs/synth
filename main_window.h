#pragma once

#include "imgui.h"
#include "implot.h"
#include <vector>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Update();
    void Draw();

private:
    void GenerateAudioSamples();
    void GenerateCustomBuffer();

    bool show_demo_window_;
    bool show_implot_demo_window_;
    bool show_another_window_;
    ImVec4 clear_color_;
    float slider_value_;
    int counter_;
    
    // Audio parameters
    float frequency_;
    float volume_;
    bool playing_;
    double phase_;
    int sample_rate_;
    std::vector<float> audio_buffer_;
    
    // Custom buffer parameters
    bool playing_custom_;
    bool regenerate_requested_;
    std::vector<float> custom_buffer_;
    int custom_buffer_size_;
};
