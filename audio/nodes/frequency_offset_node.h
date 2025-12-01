#pragma once

#include "audio/nodes/audio_node.h"
#include <vector>

namespace audio_nodes {

// Frequency Offset node: Adds a flat amount to frequency values
class FrequencyOffsetNode : public AudioNode {
public:
    FrequencyOffsetNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    float offset_;
    int input_pin_id_;
    AudioNode* input_;
    float last_output_freq_;
};

} // namespace audio_nodes
