#ifndef AUDIO_INTERFACE_H
#define AUDIO_INTERFACE_H

#include <vector>

// Interface for audio playback using OpenAL
// Handles all OpenAL library interactions
class AudioInterface {
public:
    AudioInterface();
    ~AudioInterface();

    // Initialize the audio system
    bool init();

    // Play audio samples (mono, 16-bit, at given sample rate)
    void playSamples(const std::vector<short>& samples, int sampleRate, bool looping);

    // Start playback
    void play();

    // Stop playback
    void stop();

    // Update volume (0.0 to 1.0)
    void setVolume(float volume);

    // Check if currently playing
    bool isPlaying() const { return m_isPlaying; }

    // Get the current buffer samples for visualization
    const std::vector<short>& getCurrentSamples() const { return m_currentSamples; }

private:
    void* m_device;
    void* m_context;
    unsigned int m_source;
    unsigned int m_buffer;
    bool m_isPlaying;
    std::vector<short> m_currentSamples;  // Store current buffer for visualization
};

#endif // AUDIO_INTERFACE_H
