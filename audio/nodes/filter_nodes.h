#pragma once

#include "audio/nodes/audio_node.h"
#include <vector>

namespace audio_nodes {

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

} // namespace audio_nodes
