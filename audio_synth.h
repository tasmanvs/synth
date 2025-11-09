#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

class AudioSynth {
public:
    AudioSynth();
    ~AudioSynth();
    
    // Initialize OpenAL context and device
    bool init();
    
    // Start playing a tone at the given frequency and volume
    void startTone(float frequency, float volume);
    
    // Stop playing the current tone
    void stopTone();
    
    // Update the current tone's frequency and volume
    void updateTone(float frequency, float volume);
    
    // Check if a tone is currently playing
    bool isPlaying() const { return m_isPlaying; }
    
private:
    void* m_device;      // ALCdevice*
    void* m_context;     // ALCcontext*
    unsigned int m_source;    // ALuint
    unsigned int m_buffer;    // ALuint
    bool m_isPlaying;
    
    // Generate a sine wave buffer
    void generateSineWave(float frequency, float duration, int sampleRate);
};

#endif // AUDIO_SYNTH_H
