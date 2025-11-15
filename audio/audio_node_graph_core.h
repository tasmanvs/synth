#pragma once

#include "audio/audio_synth.h"
#include "audio/audio_interface.h"
#include <vector>
#include <memory>
#include <map>

// Core audio node classes without UI dependencies for testing

namespace audio_nodes {

// Forward declarations
class AudioNode;
class SourceNode;
class SumNode;

// Audio node types
enum class NodeType {
    kSource,
    kSum,
    kPlayer
};

// Base class for all audio nodes (core functionality only)
class AudioNodeCore {
public:
    AudioNodeCore(int node_id, NodeType type);
    virtual ~AudioNodeCore() = default;
    
    // Generate audio output for this node
    virtual std::vector<float> GenerateAudio(int num_samples, int sample_rate) = 0;
    
    // Connect an input (for nodes that accept inputs)
    virtual bool AddInput(AudioNodeCore* input_node) { return false; }
    virtual void RemoveInput(AudioNodeCore* input_node) {}
    
    int GetNodeId() const { return node_id_; }
    NodeType GetNodeType() const { return type_; }
    int GetOutputPinId() const { return output_pin_id_; }
    
protected:
    int node_id_;
    int output_pin_id_;
    NodeType type_;
};

// Source node: Generates a sine wave with configurable frequency and volume
class SourceNodeCore : public AudioNodeCore {
public:
    SourceNodeCore(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    
    float GetFrequency() const { return frequency_; }
    float GetVolume() const { return volume_; }
    void SetFrequency(float freq) { frequency_ = freq; }
    void SetVolume(float vol) { volume_ = vol; }
    
    // Check if parameters have changed since last check
    bool HasParametersChanged();
    void ResetChangeFlag() { parameters_changed_ = false; }
    
private:
    float frequency_;
    float volume_;
    float phase_;  // Track phase for continuous audio generation
    float last_frequency_;
    float last_volume_;
    bool parameters_changed_;
};

// Sum node: Adds the output of two input nodes
class SumNodeCore : public AudioNodeCore {
public:
    SumNodeCore(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    
    bool AddInput(AudioNodeCore* input_node) override;
    void RemoveInput(AudioNodeCore* input_node) override;
    
private:
    int input_pin_a_id_;
    int input_pin_b_id_;
    AudioNodeCore* input_a_;
    AudioNodeCore* input_b_;
};

} // namespace audio_nodes
