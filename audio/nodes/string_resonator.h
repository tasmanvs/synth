#pragma once

#include <vector>

namespace audio_nodes {

// Generate string resonator waveform with harmonics (single frequency)
// Produces harmonics at f, 2f, 3f, 4f, etc. with 1/n amplitude falloff
std::vector<float> GenerateStringResonator(
    float fundamental_freq,
    int num_harmonics,
    float volume,
    int num_samples,
    int sample_rate,
    std::vector<double>& phases);

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
    std::vector<float>& output);

} // namespace audio_nodes
