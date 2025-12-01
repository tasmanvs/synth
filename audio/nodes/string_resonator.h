#pragma once

#include <vector>

namespace audio_nodes {

// Amplitude falloff for string resonator harmonics
enum class AmplitudeFalloff {
    kFlat,        // All harmonics have same amplitude
    kLinear,      // Linear falloff: 1, 0.9, 0.8, ...
    kOneOverX,    // 1/X falloff: 1, 1/2, 1/3, 1/4, ...
    kExponential  // Exponential falloff: e^(-x)
};

// Generate string resonator waveform with harmonics (single frequency)
// Produces harmonics at f, 2f, 3f, 4f, etc. with configurable amplitude falloff
std::vector<float> GenerateStringResonator(
    float fundamental_freq,
    int num_harmonics,
    float volume,
    int num_samples,
    int sample_rate,
    std::vector<double>& phases,
    AmplitudeFalloff falloff = AmplitudeFalloff::kOneOverX);

// Generate string resonator with per-sample frequency modulation
// base_freqs: fundamental frequency for each sample
// harmonic_phases: phases for each harmonic (will be resized if needed)
void GenerateStringResonatorWithModulation(
    const std::vector<float>& base_freqs,
    int num_harmonics,
    float volume_per_freq,
    int num_samples,
    int sample_rate,
    std::vector<double>& harmonic_phases,
    std::vector<float>& output,
    AmplitudeFalloff falloff = AmplitudeFalloff::kOneOverX);

} // namespace audio_nodes
