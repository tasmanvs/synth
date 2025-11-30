#include "audio/nodes/keyboard_frequency_node.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include <cmath>

namespace audio_nodes {

KeyboardFrequencyNode::KeyboardFrequencyNode(int node_id)
    : AudioNode(node_id, NodeType::kKeyboardFrequency)
    , base_frequency_(440.0f)
    , current_octave_(0)
    , current_note_(-1)
    , num_subdivisions_(12) {
}

std::vector<float> KeyboardFrequencyNode::GenerateAudio(int num_samples, int sample_rate) {
    // Output constant frequency value based on current octave and note
    float frequency = GetCurrentFrequency();
    return std::vector<float>(num_samples, frequency);
}

float KeyboardFrequencyNode::GetOctaveMultiplier() const {
    // Octave multiplier: 2^octave
    // Octave 0 (key "1") = 2^0 = 1
    // Octave 1 (key "2") = 2^1 = 2
    // ...
    // Octave 9 (key "0") = 2^9 = 512
    return std::pow(2.0f, static_cast<float>(current_octave_));
}

float KeyboardFrequencyNode::GetNoteMultiplier() const {
    if (current_note_ < 0) {
        return 0.0f;  // No note pressed
    }
    
    // Note multiplier: 2^(note/subdivisions)
    // This divides the octave into num_subdivisions equal parts
    // Note 0 = octave start, Note subdivisions = octave end (2x start)
    return std::pow(2.0f, static_cast<float>(current_note_) / static_cast<float>(num_subdivisions_));
}

float KeyboardFrequencyNode::GetCurrentFrequency() const {
    if (current_note_ < 0) {
        return 0.0f;  // No note pressed
    }
    
    // Frequency = base * octave_multiplier * note_multiplier
    return base_frequency_ * GetOctaveMultiplier() * GetNoteMultiplier();
}

void KeyboardFrequencyNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Keyboard Frequency %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    
    ImGui::SliderFloat("Base", &base_frequency_, 0.1f, 2000.0f, "%.1f Hz");
    ImGui::SliderInt("Divisions", &num_subdivisions_, 2, 12);
    
    ImGui::Spacing();
    
    // Display current octave, note and frequency
    const char* octave_names[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    const char* note_names[] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]"};
    
    ImGui::Text("Octave: %s (x%.0f)", octave_names[current_octave_], GetOctaveMultiplier());
    
    if (current_note_ >= 0) {
        ImGui::Text("Note: %s (%d/%d)", note_names[current_note_], current_note_ + 1, num_subdivisions_);
        ImGui::Text("Freq: %.1f Hz", GetCurrentFrequency());
    } else {
        ImGui::Text("Note: None");
        ImGui::Text("Freq: 0.0 Hz");
    }
    
    ImGui::Spacing();
    
    // Visual keyboard representation
    const char* key_labels[] = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]"};
    ImGui::Text("Keys:");
    for (int i = 0; i < num_subdivisions_; i++) {
        if (i > 0) ImGui::SameLine();
        
        // Highlight the currently pressed key
        if (i == current_note_) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }
        
        ImGui::Button(key_labels[i], ImVec2(20, 20));
        ImGui::PopStyleColor(3);
    }
    
    ImGui::Spacing();
    
    // Handle octave selection (sticky - keys 1-0)
    if (ImGui::IsKeyPressed(ImGuiKey_1)) current_octave_ = 0;
    else if (ImGui::IsKeyPressed(ImGuiKey_2)) current_octave_ = 1;
    else if (ImGui::IsKeyPressed(ImGuiKey_3)) current_octave_ = 2;
    else if (ImGui::IsKeyPressed(ImGuiKey_4)) current_octave_ = 3;
    else if (ImGui::IsKeyPressed(ImGuiKey_5)) current_octave_ = 4;
    else if (ImGui::IsKeyPressed(ImGuiKey_6)) current_octave_ = 5;
    else if (ImGui::IsKeyPressed(ImGuiKey_7)) current_octave_ = 6;
    else if (ImGui::IsKeyPressed(ImGuiKey_8)) current_octave_ = 7;
    else if (ImGui::IsKeyPressed(ImGuiKey_9)) current_octave_ = 8;
    else if (ImGui::IsKeyPressed(ImGuiKey_0)) current_octave_ = 9;
    
    // Handle note selection (active while held - keys q-])
    // Map keys q-] (12 keys) to note indices 0-11
    // Only the first num_subdivisions_ keys are active
    const ImGuiKey note_keys[] = {
        ImGuiKey_Q, ImGuiKey_W, ImGuiKey_E, ImGuiKey_R, 
        ImGuiKey_T, ImGuiKey_Y, ImGuiKey_U, ImGuiKey_I, 
        ImGuiKey_O, ImGuiKey_P, ImGuiKey_LeftBracket, ImGuiKey_RightBracket
    };
    
    // Check if any valid note key is held
    current_note_ = -1;
    for (int i = 0; i < num_subdivisions_; i++) {
        if (ImGui::IsKeyDown(note_keys[i])) {
            current_note_ = i;
            break;  // Only play one note at a time
        }
    }
    
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

} // namespace audio_nodes
