#pragma once

#include "imgui.h"
#include "implot.h"
#include "imgui_node_editor.h"
#include "audio/audio_synth.h"
#include "audio/audio_interface.h"
#include <vector>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Update();
    void Draw();

private:
    bool show_demo_window_;
    bool show_implot_demo_window_;
    bool show_another_window_;
    bool show_node_editor_window_;
    ImVec4 clear_color_;
    float slider_value_;
    int counter_;
    
    // Audio parameters
    float frequency_;
    float volume_;
    bool playing_;
    int sample_rate_;
    
    // Audio components
    AudioSynth audio_synth_;          // Generates waveforms
    AudioInterface audio_interface_;   // Handles playback
    
    // Node editor
    ax::NodeEditor::EditorContext* node_editor_context_;
    bool node_editor_initialized_;
    
    void DrawNodeEditorDemo();
};
