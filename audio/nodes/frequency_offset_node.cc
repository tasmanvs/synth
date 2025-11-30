#include "audio/nodes/frequency_offset_node.h"
#include "imgui.h"
#include "imgui_node_editor.h"

namespace audio_nodes {

FrequencyOffsetNode::FrequencyOffsetNode(int node_id)
    : AudioNode(node_id, NodeType::kFrequencyOffset)
    , offset_(0.0f)
    , input_pin_id_(node_id * 100 + 2)
    , input_(nullptr)
    , last_output_freq_(0.0f) {
}

std::vector<float> FrequencyOffsetNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_) {
        last_output_freq_ = 0.0f;
        return std::vector<float>(num_samples, 0.0f);
    }
    
    std::vector<float> input_values = input_->GenerateAudio(num_samples, sample_rate);
    
    // Add offset to each sample, but if input is 0, output is 0
    for (float& value : input_values) {
        if (value == 0.0f) {
            // Input is 0, output should be 0
            value = 0.0f;
        } else {
            value += offset_;
        }
    }
    
    // Store the last output frequency for display
    if (!input_values.empty()) {
        last_output_freq_ = input_values.back();
    }
    
    return input_values;
}

bool FrequencyOffsetNode::AddInput(AudioNode* input_node, int pin_id) {
    if (pin_id != input_pin_id_) return false;
    input_ = input_node;
    return true;
}

void FrequencyOffsetNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node && pin_id == input_pin_id_) {
        input_ = nullptr;
    }
}

void FrequencyOffsetNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Frequency Offset %d", node_id_);
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    
    ImGui::PushItemWidth(120.0f);
    ImGui::DragFloat("Offset (Hz)", &offset_, 0.1f, -10000.0f, 10000.0f, "%.1f");
    ImGui::PopItemWidth();
    
    // Display output frequency
    ImGui::Text("Output: %.1f Hz", last_output_freq_);
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

} // namespace audio_nodes
