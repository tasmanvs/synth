#pragma once

#include "audio/nodes/audio_node.h"
#include "audio_loop/sine_buffer_generator.h"
#include <vector>

namespace audio_nodes {

// Harmonic node: Generates a base frequency plus integer multiples (harmonics)
class HarmonicNode : public AudioNode {
public:
    HarmonicNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    float GetBaseFrequency() const { return base_frequency_; }
    float GetVolume() const { return volume_; }
    int GetNumHarmonics() const { return num_harmonics_; }
    
    // Check if parameters have changed since last check
    bool HasParametersChanged();
    void ResetChangeFlag() { parameters_changed_ = false; }
    
private:
    float base_frequency_;
    float volume_;
    int num_harmonics_;
    float last_base_frequency_;
    float last_volume_;
    int last_num_harmonics_;
    bool parameters_changed_;
    std::vector<audio_loop::BufferConfig> harmonic_configs_;
    std::vector<audio_loop::PhaseContinuousSine> harmonic_generators_;
    
    void UpdateHarmonics();
};

} // namespace audio_nodes
