#pragma once

#include "audio/nodes/audio_node.h"
#include "audio_loop/sine_buffer_generator.h"
#include <vector>

namespace audio_nodes {

// Waveform types for source node
enum class WaveformType {
    kSine,
    kSawtooth,
    kSquare,
    kSmoothedSquare,
    kStringResonator
};

// Source node: Generates waveforms (sine, sawtooth, square) with configurable frequency and volume
class SourceNode : public AudioNode {
public:
    SourceNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
    bool AddInput(AudioNode* input_node, int pin_id = -1) override;
    void RemoveInput(AudioNode* input_node, int pin_id = -1) override;
    
    float GetFrequency() const { return frequency_; }
    float GetVolume() const { return volume_; }
    
    // Setters for testing
    void SetWaveformType(WaveformType type) { waveform_type_ = type; }
    void SetFrequency(float freq) { frequency_ = freq; }
    void SetFrequencyEnd(float freq) { frequency_end_ = freq; }
    void SetFrequencyCount(int count) { frequency_count_ = count; phases_.resize(count, 0.0); }
    void SetEndFrequencyInclusive(bool inclusive) { end_frequency_inclusive_ = inclusive; }
    void SetVolume(float vol) { volume_ = vol; }
    void SetNumHarmonics(int num) { num_harmonics_ = num; }
    
    // Check if parameters have changed since last check
    bool HasParametersChanged();
    void ResetChangeFlag() { parameters_changed_ = false; }
    
private:
    float frequency_;
    float volume_;
    WaveformType waveform_type_;
    float last_frequency_;
    float last_volume_;
    WaveformType last_waveform_type_;
    bool parameters_changed_;
    audio_loop::BufferConfig buffer_config_;
    std::vector<double> phases_; // Phase accumulators for each frequency
    int num_harmonics_; // Number of harmonics for string resonator
    float smoothing_time_; // Smoothing time in milliseconds for smoothed square wave
    
    // Multi-frequency support
    float frequency_end_;
    int frequency_count_;
    float last_frequency_end_;
    int last_frequency_count_;
    bool end_frequency_inclusive_;
    
    int input_pin_id_start_;
    int input_pin_id_end_;
    AudioNode* input_start_;
    AudioNode* input_end_;
    
    std::vector<float> GenerateSine(int num_samples, int sample_rate);
    std::vector<float> GenerateSawtooth(int num_samples, int sample_rate);
    std::vector<float> GenerateSquare(int num_samples, int sample_rate);
    std::vector<float> GenerateSmoothedSquare(int num_samples, int sample_rate);
    std::vector<float> GenerateStringResonator(int num_samples, int sample_rate);
};

} // namespace audio_nodes
