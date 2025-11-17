#pragma once

#include "imgui.h"
#include "implot.h"
#include "imgui_node_editor.h"
#include "audio/audio_interface.h"
#include "audio_loop/sine_buffer_generator.h"
#include <vector>
#include <memory>
#include <map>

namespace audio_nodes {

// Forward declarations
class AudioNode;
class SourceNode;
class SumNode;
class PlayerNode;

// Audio node types
enum class NodeType {
    kSource,
    kSum,
    kPlayer
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
    virtual bool AddInput(AudioNode* input_node) { return false; }
    virtual void RemoveInput(AudioNode* input_node) {}
    
    int GetNodeId() const { return node_id_; }
    NodeType GetNodeType() const { return type_; }
    
    int GetOutputPinId() const { return output_pin_id_; }
    
protected:
    int node_id_;
    int output_pin_id_;
    NodeType type_;
};

// Source node: Generates a sine wave with configurable frequency and volume
class SourceNode : public AudioNode {
public:
    SourceNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    float GetFrequency() const { return frequency_; }
    float GetVolume() const { return volume_; }
    
    // Check if parameters have changed since last check
    bool HasParametersChanged();
    void ResetChangeFlag() { parameters_changed_ = false; }
    
private:
    float frequency_;
    float volume_;
    float last_frequency_;
    float last_volume_;
    bool parameters_changed_;
    audio_loop::BufferConfig buffer_config_;
    audio_loop::PhaseContinuousSine phase_generator_;
};

// Sum node: Adds the output of two input nodes
class SumNode : public AudioNode {
public:
    SumNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node) override;
    void RemoveInput(AudioNode* input_node) override;
    
private:
    int input_pin_a_id_;
    int input_pin_b_id_;
    AudioNode* input_a_;
    AudioNode* input_b_;
};

// Player node: Outputs audio to speakers (singleton)
class PlayerNode : public AudioNode {
public:
    PlayerNode(int node_id, AudioInterface* audio_interface);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node) override;
    void RemoveInput(AudioNode* input_node) override;
    
    void SetPlaying(bool playing);
    bool IsPlaying() const { return playing_; }
    
    // Update the audio output (call this regularly)
    void UpdateAudio(int sample_rate);
    
private:
    int input_pin_id_;
    AudioNode* input_;
    AudioInterface* audio_interface_;
    bool playing_;
    float volume_;
    std::vector<float> playback_history_;
    size_t history_limit_samples_;
    bool show_debug_window_;
    std::vector<float> plot_scratch_buffer_;
    size_t max_plot_samples_;

    void AppendToHistory(const std::vector<float>& samples);
    void DrawHistoryWindow();
    const float* PreparePlotData(int* sample_count);
};

// Audio node graph manager
class AudioNodeGraph {
public:
    AudioNodeGraph(AudioInterface* audio_interface);
    ~AudioNodeGraph();
    
    // Node creation
    SourceNode* CreateSourceNode();
    SumNode* CreateSumNode();
    PlayerNode* CreatePlayerNode();
    
    // Node management
    void DeleteNode(int node_id);
    AudioNode* GetNode(int node_id);
    
    // Link management
    bool CreateLink(int start_pin_id, int end_pin_id);
    void DeleteLink(int link_id);
    
    // Get pin information
    int GetNodeIdForPin(int pin_id);
    AudioNode* GetNodeForPin(int pin_id);
    bool IsPinOutput(int pin_id);
    
    // Update and draw
    void Update(int sample_rate);
    void Draw();
    
    // Check if any parameters have changed
    bool HasGraphChanged();
    
    // Access the editor context
    ax::NodeEditor::EditorContext* GetEditorContext() { return editor_context_; }
    
private:
    struct Link {
        int link_id;
        int start_pin_id;
        int end_pin_id;
    };
    
    std::map<int, std::unique_ptr<AudioNode>> nodes_;
    std::vector<Link> links_;
    std::map<int, int> pin_to_node_map_;
    
    AudioInterface* audio_interface_;
    PlayerNode* player_node_;
    ax::NodeEditor::EditorContext* editor_context_;
    
    int next_node_id_;
    int next_link_id_;
    
    bool editor_initialized_;
    
    void RegisterPin(int pin_id, int node_id);
    void UnregisterPinsForNode(int node_id);
};

} // namespace audio_nodes
