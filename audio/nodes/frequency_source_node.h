#pragma once

#include "audio/nodes/audio_node.h"
#include <vector>

namespace audio_nodes {

// Frequency Source node: Generates a constant frequency value
class FrequencySourceNode : public AudioNode {
public:
    FrequencySourceNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
private:
    float frequency_;
};

} // namespace audio_nodes
