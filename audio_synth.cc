#include "audio_synth.h"
#include "absl/log/log.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioSynth::AudioSynth() {
}

AudioSynth::~AudioSynth() {
}

std::vector<short> AudioSynth::generateSineWave(float frequency, float duration, 
                                                int sampleRate, float volume) {
    // Calculate number of samples
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<short> samples(numSamples);
    
    LOG(INFO) << "Generating sine wave: " << frequency << " Hz, " 
              << duration << "s, " << numSamples << " samples";
    
    // Generate sine wave
    for (int i = 0; i < numSamples; i++) {
        float t = static_cast<float>(i) / sampleRate;
        float value = std::sin(2.0f * M_PI * frequency * t);
        samples[i] = static_cast<short>(value * 32767.0f * volume);
    }
    
    return samples;
}

std::vector<short> AudioSynth::generateSineWaveCycle(float frequency, int sampleRate, 
                                                     float volume) {
    // Calculate samples needed for exactly one cycle
    // This ensures seamless looping
    float cycleTime = 1.0f / frequency;
    int numSamples = static_cast<int>(cycleTime * sampleRate);
    
    // Ensure at least a minimum number of samples for quality
    if (numSamples < 10) {
        numSamples = 10;
    }
    
    std::vector<short> samples(numSamples);
    
    LOG(INFO) << "Generating sine wave cycle: " << frequency << " Hz, " 
              << numSamples << " samples per cycle";
    
    // Generate one complete cycle
    for (int i = 0; i < numSamples; i++) {
        float phase = static_cast<float>(i) / numSamples; // 0 to 1
        float value = std::sin(2.0f * M_PI * phase);
        samples[i] = static_cast<short>(value * 32767.0f * volume);
    }
    
    return samples;
}
