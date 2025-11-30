#pragma once

#include <vector>

namespace audio_nodes {

// Audio node types
enum class NodeType {
    kSource,
    kSum,
    kPlayer,
    kHarmonic,
    kBandpassFilter,
    kLowpassFilter,
    kHighpassFilter,
    kWhiteNoise,
    kNormalizer,
    kAmplitudeModulator,
    kScaler,
    kReverb,
    kPitchShifter,
    kFrequencySource,
    kKeyboardFrequency,
    kFrequencyOffset
};

// Base class for all audio nodes
class AudioNode {
public:
    AudioNode(int node_id, NodeType type);
    virtual ~AudioNode() = default;
    
    // Generate audio output for this node
    virtual std::vector<float> GenerateAudio(int num_samples, int sample_rate) = 0;
    
    // Draw the node in the editor
    virtual void Draw() = 0;
    
    // Connect an input (for nodes that accept inputs)
    virtual bool AddInput(AudioNode* input_node, int pin_id = -1) { return false; }
    virtual void RemoveInput(AudioNode* input_node, int pin_id = -1) {}
    
    int GetNodeId() const { return node_id_; }
    NodeType GetNodeType() const { return type_; }
    
    int GetOutputPinId() const { return output_pin_id_; }
    
protected:
    int node_id_;
    int output_pin_id_;
    NodeType type_;
};

} // namespace audio_nodes
