#pragma once

#include <deque>
#include <vector>

// Interface for audio playback using OpenAL
// Handles all OpenAL library interactions
class AudioInterface {
public:
    AudioInterface();
    ~AudioInterface();

    // Initialize the audio system
    bool Init();

    // Play audio samples (mono, 16-bit, at given sample rate)
    void PlaySamples(const std::vector<short>& samples, int sample_rate, bool looping);

    // Start playback
    void Play();

    // Stop playback
    void Stop();

    // Update volume (0.0 to 1.0)
    void SetVolume(float volume);

    // Streaming queue helpers
    bool AppendSamples(const std::vector<short>& samples, int sample_rate, bool end_stream);
    void ServiceStreamingQueue();
    void ClearStreamingQueue();
    int GetQueuedBufferCount() const;

    // Check if currently playing
    bool IsPlaying() const { return is_playing_; }

    // Get the current buffer samples for visualization
    const std::vector<short>& GetCurrentSamples() const { return current_samples_; }

private:
    bool QueueStreamingBuffer(const std::vector<short>& samples, int sample_rate);
    void UnqueueProcessedBuffers();
    void DeleteAllQueuedBuffers();

    void* device_;
    void* context_;
    unsigned int source_;
    unsigned int buffer_;
    bool is_playing_;
    std::vector<short> current_samples_;  // Store current buffer for visualization
    std::deque<unsigned int> queued_buffers_;
    bool streaming_end_pending_;
    int streaming_sample_rate_;
};
