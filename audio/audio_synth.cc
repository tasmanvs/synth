#include "audio/audio_synth.h"
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
                                                int sample_rate, float volume) {
    // Calculate number of samples
    int num_samples = static_cast<int>(duration * sample_rate);
    std::vector<short> samples(num_samples);
    
    LOG(INFO) << "Generating sine wave: " << frequency << " Hz, " 
              << duration << "s, " << num_samples << " samples";
    
    // Generate sine wave
    for (int i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        float value = std::sin(2.0f * M_PI * frequency * t);
        samples[i] = static_cast<short>(value * 32767.0f * volume);
    }
    
    return samples;
}

std::vector<short> AudioSynth::generateSineWaveCycle(float frequency, int sample_rate, 
                                                     float volume) {
    // Calculate samples needed for exactly one cycle
    // This ensures seamless looping
    float cycle_time = 1.0f / frequency;
    int num_samples = static_cast<int>(cycle_time * sample_rate);
    
    // Ensure at least a minimum number of samples for quality
    if (num_samples < 10) {
        num_samples = 10;
    }
    
    std::vector<short> samples(num_samples);
    
    LOG(INFO) << "Generating sine wave cycle: " << frequency << " Hz, " 
              << num_samples << " samples per cycle";
    
    // Generate one complete cycle
    for (int i = 0; i < num_samples; i++) {
        float phase = static_cast<float>(i) / num_samples; // 0 to 1
        float value = std::sin(2.0f * M_PI * phase);
        samples[i] = static_cast<short>(value * 32767.0f * volume);
    }
    
    return samples;
}
