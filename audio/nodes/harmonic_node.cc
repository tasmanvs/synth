#include "audio/nodes/harmonic_node.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include <algorithm>

namespace audio_nodes {

HarmonicNode::HarmonicNode(int node_id)
    : AudioNode(node_id, NodeType::kHarmonic)
    , base_frequency_(220.0f)
    , volume_(0.3f)
    , num_harmonics_(5)
    , last_base_frequency_(220.0f)
    , last_volume_(0.3f)
    , last_num_harmonics_(5)
    , parameters_changed_(false) {
    UpdateHarmonics();
}

void HarmonicNode::UpdateHarmonics() {
    // Preserve existing generators to maintain phase continuity
    int current_count = static_cast<int>(harmonic_generators_.size());
    
    if (num_harmonics_ > current_count) {
        // Add new generators
        for (int i = current_count; i < num_harmonics_; ++i) {
            audio_loop::BufferConfig config;
            config.frequency_hz = base_frequency_ * (i + 1);
            config.amplitude = volume_ / static_cast<float>(num_harmonics_);
            config.sample_rate = 48000;
            config.frame_count = 512;
            
            harmonic_configs_.push_back(config);
            harmonic_generators_.emplace_back(0.0f);
        }
    } else if (num_harmonics_ < current_count) {
        // Remove excess generators
        harmonic_configs_.resize(num_harmonics_);
        harmonic_generators_.resize(num_harmonics_);
    }
    
    // Update existing configs (generators maintain their phase)
    for (int i = 0; i < num_harmonics_; ++i) {
        if (i < static_cast<int>(harmonic_configs_.size())) {
            harmonic_configs_[i].frequency_hz = base_frequency_ * (i + 1);
            harmonic_configs_[i].amplitude = volume_ / static_cast<float>(num_harmonics_);
        }
    }
}

std::vector<float> HarmonicNode::GenerateAudio(int num_samples, int sample_rate) {
    if (num_samples <= 0 || sample_rate <= 0) {
        return {};
    }
    
    std::vector<float> output(num_samples, 0.0f);
    
    // Generate and sum all harmonics
    for (size_t i = 0; i < harmonic_generators_.size(); ++i) {
        harmonic_configs_[i].frame_count = num_samples;
        harmonic_configs_[i].sample_rate = sample_rate;
        harmonic_configs_[i].frequency_hz = base_frequency_ * (i + 1);
        harmonic_configs_[i].amplitude = volume_ / static_cast<float>(num_harmonics_);
        
        auto harmonic_buffer = harmonic_generators_[i].GenerateBuffer(harmonic_configs_[i]);
        
        for (int j = 0; j < num_samples && j < static_cast<int>(harmonic_buffer.size()); ++j) {
            output[j] += harmonic_buffer[j];
        }
    }
    
    // Clamp to prevent overflow
    for (int i = 0; i < num_samples; ++i) {
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
    }
    
    return output;
}

void HarmonicNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Harmonic Node %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    if (ImGui::SliderFloat("Base Freq", &base_frequency_, 20.0f, 1000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderInt("Harmonics", &num_harmonics_, 1, 16)) {
        parameters_changed_ = true;
        UpdateHarmonics();
    }
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

bool HarmonicNode::HasParametersChanged() {
    if (base_frequency_ != last_base_frequency_ || 
        volume_ != last_volume_ || 
        num_harmonics_ != last_num_harmonics_) {
        last_base_frequency_ = base_frequency_;
        last_volume_ = volume_;
        last_num_harmonics_ = num_harmonics_;
        return true;
    }
    return parameters_changed_;
}

} // namespace audio_nodes
