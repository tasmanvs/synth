#pragma once


#include "imgui_node_editor.h"
#include "audio/audio_interface.h"
#include "audio_loop/sine_buffer_generator.h"
#include <vector>
#include <memory>
#include <map>
#include <deque>

namespace audio_nodes {

// Forward declarations
class AudioNode;
class SourceNode;
class SumNode;
class PlayerNode;
class BandpassFilterNode;
class LowpassFilterNode;
class HighpassFilterNode;
class WhiteNoiseNode;
class AudioNodeGraph;

// Audio node types
enum class NodeType {
    kSource,
    kSum,
    kPlayer,
    kHarmonic,
    kBandpassFilter,
    kLowpassFilter,
    kHighpassFilter,
    kWhiteNoise
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

// Bandpass filter node: Filters input signal to a specific frequency range
class BandpassFilterNode : public AudioNode {
public:
    BandpassFilterNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float center_frequency_;
    float bandwidth_;
    
    // Biquad filter coefficients
    float b0_, b1_, b2_, a1_, a2_;
    // Filter state (for continuous processing)
    float x1_, x2_, y1_, y2_;
    
    void UpdateFilterCoefficients(int sample_rate);
    float ProcessSample(float input);
};

// Lowpass filter node: Filters out high frequencies
class LowpassFilterNode : public AudioNode {
public:
    LowpassFilterNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float cutoff_frequency_;
    int filter_order_;
    bool show_bode_plot_;
    
    // Dynamic cascaded biquad filter coefficients
    std::vector<float> b0_, b1_, b2_;
    std::vector<float> a1_, a2_;
    // Filter state for each stage
    std::vector<float> x1_, x2_;
    std::vector<float> y1_, y2_;
    
    void UpdateFilterCoefficients(int sample_rate);
    void ResizeFilterArrays();
    float ProcessSample(float input);
    float ComputeFrequencyResponse(float frequency, int sample_rate);
};

// Highpass filter node: Filters out low frequencies
class HighpassFilterNode : public AudioNode {
public:
    HighpassFilterNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float cutoff_frequency_;
    bool show_bode_plot_;
    
    // Cascaded biquad filter coefficients (4 stages for 8th order filter)
    static const int kNumStages_ = 4;
    float b0_[kNumStages_], b1_[kNumStages_], b2_[kNumStages_];
    float a1_[kNumStages_], a2_[kNumStages_];
    // Filter state for each stage
    float x1_[kNumStages_], x2_[kNumStages_];
    float y1_[kNumStages_], y2_[kNumStages_];
    
    void UpdateFilterCoefficients(int sample_rate);
    float ProcessSample(float input);
    float ComputeFrequencyResponse(float frequency, int sample_rate);
};

// White noise node: Generates random white noise
class WhiteNoiseNode : public AudioNode {
public:
    WhiteNoiseNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
private:
    float volume_;
};

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

// Player node: Outputs audio to speakers (singleton)
class PlayerNode : public AudioNode {
public:
    PlayerNode(int node_id, AudioInterface* audio_interface);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
    void SetPlaying(bool playing);
    bool IsPlaying() const { return playing_; }
    
    // Update the audio output (call this regularly)
    void UpdateAudio(int sample_rate);
    void OnGraphChanged();
    
    // Draw spectrogram content (without window wrapper)
    void DrawSpectrogramContent();
    
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
    std::vector<short> pcm_convert_buffer_;
    int streaming_buffer_size_;
    int max_queue_buffers_;
    bool needs_stream_prime_;
    std::vector<float> capture_buffer_;
    size_t capture_target_samples_;
    size_t capture_samples_collected_;
    bool capture_active_;
    bool capture_ready_;
    int capture_target_input_;
    std::deque<std::vector<float>> spectrogram_data_;
    size_t spectrogram_time_slices_;
    int fft_size_;
    std::vector<float> fft_window_;
    std::vector<float> fft_input_buffer_;
    int sample_rate_;
    int spectrogram_sample_counter_;
    double frequency_axis_min_;
    double frequency_axis_max_;

    void AppendToHistory(const std::vector<float>& samples);
    void DrawHistoryWindow();
    const float* PreparePlotData(const std::vector<float>& samples, int* sample_count);
    void UpdateStreaming(int sample_rate);
    bool QueueGeneratedAudio(int sample_rate);
    void StartCapture();
    void AppendCaptureSamples(const std::vector<float>& samples);
    void ComputeFFT(const float* input, int size, std::vector<float>& magnitudes);
    void UpdateSpectrogram(const std::vector<float>& samples);
};

// Audio node graph manager
class AudioNodeGraph {
public:
    AudioNodeGraph(AudioInterface* audio_interface);
    ~AudioNodeGraph();
    
    // Node creation
    SourceNode* CreateSourceNode();
    HarmonicNode* CreateHarmonicNode();
    SumNode* CreateSumNode();
    PlayerNode* CreatePlayerNode();
    BandpassFilterNode* CreateBandpassFilterNode();
    LowpassFilterNode* CreateLowpassFilterNode();
    HighpassFilterNode* CreateHighpassFilterNode();
    WhiteNoiseNode* CreateWhiteNoiseNode();
    
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
    
    // Access nodes (for external UI)
    const std::map<int, std::unique_ptr<AudioNode>>& GetNodes() const { return nodes_; }
    
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

    friend class SumNode;
};

} // namespace audio_nodes
