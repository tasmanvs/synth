#ifndef AUDIO_SYNTH_H
#define AUDIO_SYNTH_H

#include <vector>

// Pure waveform generator - handles synthesis only, no playback
class AudioSynth {
public:
    AudioSynth();
    ~AudioSynth();
    
    // Generate a sine wave with given parameters
    // Returns mono 16-bit samples
    std::vector<short> generateSineWave(float frequency, float duration, 
                                        int sampleRate, float volume);
    
    // Generate a single cycle of a sine wave (for looping)
    std::vector<short> generateSineWaveCycle(float frequency, int sampleRate, 
                                            float volume);
};

#endif // AUDIO_SYNTH_H
