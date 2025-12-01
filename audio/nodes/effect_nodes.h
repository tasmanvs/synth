#pragma once

#include "audio/nodes/audio_node.h"
#include <vector>

namespace audio_nodes {

// White noise node: Generates random white noise
class WhiteNoiseNode : public AudioNode {
public:
    WhiteNoiseNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
private:
    float volume_;
};

// Normalizer node: Dynamically applies gain to normalize the incoming signal
class NormalizerNode : public AudioNode {
public:
    NormalizerNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float target_level_;       // Target peak level (0.0 to 1.0)
    float attack_time_;        // Attack time in seconds
    float release_time_;       // Release time in seconds
    float current_gain_;       // Current applied gain
    float peak_level_;         // Tracked peak level
    float smoothing_factor_;   // For exponential moving average
};

// Amplitude Modulator node: Modulates the amplitude of carrier signal by modulator signal
class AmplitudeModulatorNode : public AudioNode {
public:
    AmplitudeModulatorNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int carrier_pin_id_;      // Pin for carrier signal input
    int modulator_pin_id_;    // Pin for modulator signal input
    AudioNode* carrier_input_;
    AudioNode* modulator_input_;
    float modulation_depth_;  // 0.0 to 1.0 - how much modulation is applied
    float dc_offset_;         // DC offset for modulator (0.5 = unipolar, 0.0 = bipolar)
};

// Scaler node: Remaps input signal from input range to output range
class ScalerNode : public AudioNode {
public:
    ScalerNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float input_min_;         // Expected input minimum
    float input_max_;         // Expected input maximum
    float output_min_;        // Desired output minimum
    float output_max_;        // Desired output maximum
    bool auto_detect_range_;  // Automatically detect input range
    float detected_min_;      // Running minimum for auto-detection
    float detected_max_;      // Running maximum for auto-detection
};

// Reverb node: Adds reverb effect using Schroeder algorithm
class ReverbNode : public AudioNode {
public:
    ReverbNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float room_size_;         // Room size parameter (0-1)
    float damping_;           // High frequency damping (0-1)
    float wet_level_;         // Wet signal level (0-1)
    float dry_level_;         // Dry signal level (0-1)
    
    // Comb filters (parallel)
    static constexpr int kNumCombs_ = 4;
    std::vector<std::vector<float>> comb_buffers_;
    std::vector<int> comb_indices_;
    std::vector<float> comb_feedback_;
    std::vector<float> comb_damp_;
    
    // Allpass filters (series)
    static constexpr int kNumAllpass_ = 2;
    std::vector<std::vector<float>> allpass_buffers_;
    std::vector<int> allpass_indices_;
    
    void InitializeBuffers(int sample_rate);
    bool buffers_initialized_;
    int last_sample_rate_;
};

// Pitch Shifter node: Shifts pitch using delay-line granulation
class PitchShifterNode : public AudioNode {
public:
    PitchShifterNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
private:
    int input_pin_id_;
    AudioNode* input_;
    float semitones_;
    
    // Delay line state
    std::vector<float> delay_buffer_;
    int write_index_;
    double phasor_;
    static constexpr int kBufferSize = 48000; // 1 second buffer
    static constexpr float kWindowSize = 0.05f; // 50ms window
    
    float ReadBuffer(double index);
};

} // namespace audio_nodes
