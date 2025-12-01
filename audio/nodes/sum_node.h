#pragma once

#include "audio/nodes/audio_node.h"
#include <vector>

namespace audio_nodes {

class AudioNodeGraph;

// Sum node: Adds the output of one or more input nodes
class SumNode : public AudioNode {
public:
    SumNode(int node_id, AudioNodeGraph* graph);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    struct InputSlot {
        int pin_id;
        AudioNode* input;
    };

    std::vector<InputSlot> inputs_;
    AudioNodeGraph* graph_;
    int next_pin_offset_;

    InputSlot* FindSlotByPin(int pin_id);
    bool AddInputSlot();
};

} // namespace audio_nodes
