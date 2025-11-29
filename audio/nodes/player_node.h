#pragma once

#include "audio/nodes/audio_node.h"
#include "audio/audio_interface.h"
#include <vector>
#include <deque>

namespace audio_nodes {

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

} // namespace audio_nodes
