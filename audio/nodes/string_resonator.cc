#include "audio/nodes/string_resonator.h"
#include <cmath>

namespace audio_nodes {

const double kPi = 3.14159265358979323846;

std::vector<float> GenerateStringResonator(
    float fundamental_freq,
    int num_harmonics,
    float volume,
    int num_samples,
    int sample_rate,
    std::vector<double>& phases) {
    
    std::vector<float> output(num_samples, 0.0f);
    
    // Ensure phases vector matches harmonics count
    if (static_cast<int>(phases.size()) != num_harmonics) {
        phases.resize(num_harmonics, 0.0);
    }
    
    // Generate harmonics at f, 2f, 3f, 4f, etc.
    for (int harmonic = 1; harmonic <= num_harmonics; ++harmonic) {
        // Amplitude decreases with higher harmonics (1/n falloff)
        float harmonic_amplitude = volume / static_cast<float>(harmonic);
        double& harmonic_phase = phases[harmonic - 1];
        
        for (int i = 0; i < num_samples; ++i) {
            double harmonic_freq = fundamental_freq * static_cast<double>(harmonic);
            double phase_increment = 2.0 * kPi * harmonic_freq / sample_rate;
            
            output[i] += harmonic_amplitude * std::sin(harmonic_phase);
            harmonic_phase += phase_increment;
            
            // Wrap phase
            if (harmonic_phase >= 2.0 * kPi) {
                harmonic_phase -= 2.0 * kPi;
            }
        }
    }
    
    return output;
}

void GenerateStringResonatorWithModulation(
    const std::vector<float>& base_freqs,
    int num_harmonics,
    float volume_per_freq,
    int num_samples,
    int sample_rate,
    std::vector<double>& harmonic_phases,
    std::vector<float>& output) {
    
    // Ensure phases vector matches harmonics count
    if (static_cast<int>(harmonic_phases.size()) != num_harmonics) {
        harmonic_phases.resize(num_harmonics, 0.0);
    }
    
    // Generate harmonics at f, 2f, 3f, 4f, etc.
    for (int harmonic = 1; harmonic <= num_harmonics; ++harmonic) {
        // Amplitude decreases with higher harmonics (1/n falloff)
        float harmonic_amplitude = volume_per_freq / static_cast<float>(harmonic);
        double& harmonic_phase = harmonic_phases[harmonic - 1];
        
        for (int i = 0; i < num_samples; ++i) {
            double harmonic_freq = base_freqs[i] * static_cast<double>(harmonic);
            double phase_increment = 2.0 * kPi * harmonic_freq / sample_rate;
            
            output[i] += harmonic_amplitude * std::sin(harmonic_phase);
            harmonic_phase += phase_increment;
            
            // Wrap phase
            if (harmonic_phase >= 2.0 * kPi) {
                harmonic_phase -= 2.0 * kPi;
            }
        }
    }
}

} // namespace audio_nodes
