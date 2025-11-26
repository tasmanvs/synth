#pragma once

#include "imgui.h"
#include "implot.h"
#include "audio/audio_interface.h"
#include "audio/audio_node_graph.h"
#include <memory>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Update();
    void Draw();

private:
    bool show_demo_window_;
    bool show_implot_demo_window_;
    bool show_audio_nodes_window_;
    ImVec4 clear_color_;
    
    // Audio parameters
    float frequency_;
    float volume_;
    bool playing_;
    int sample_rate_;
    
    // Audio components
    AudioInterface audio_interface_;   // Handles playback
    
    // Audio node graph
    std::unique_ptr<audio_nodes::AudioNodeGraph> audio_node_graph_;
    
    bool dockspace_initialized_;
    bool show_spectrogram_window_;
    
    void DrawAudioNodesWindow();
    void DrawSpectrogramWindow();
};
