#pragma once

#include <vector>

// Pure waveform generator - handles synthesis only, no playback
class AudioSynth {
public:
    AudioSynth();
    ~AudioSynth();
    
    // Generate a single cycle of a sine wave (for looping)
    std::vector<short> generateSineWaveCycle(float frequency, int sampleRate, 
                                            float volume);
};
