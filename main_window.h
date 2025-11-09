#pragma once

#include "imgui.h"
#include "implot.h"
#include "audio_synth.h"
#include <vector>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Update();
    void Draw();

private:
    void GenerateAudioSamples();

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
    AudioSynth audio_synth_;
};
