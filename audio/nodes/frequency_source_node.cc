#include "audio/nodes/frequency_source_node.h"
#include "imgui.h"
#include "imgui_node_editor.h"

namespace audio_nodes {

FrequencySourceNode::FrequencySourceNode(int node_id)
    : AudioNode(node_id, NodeType::kFrequencySource)
    , frequency_(440.0f) {
}

std::vector<float> FrequencySourceNode::GenerateAudio(int num_samples, int sample_rate) {
    // Output constant frequency value
    return std::vector<float>(num_samples, frequency_);
}

void FrequencySourceNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Frequency Source %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    
    ImGui::SliderFloat("Freq", &frequency_, 0.1f, 2000.0f, "%.1f Hz");
    
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

} // namespace audio_nodes
