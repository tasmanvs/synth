// Simple standalone test for string resonator
#include "audio/nodes/string_resonator.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <complex>

const double kPi = 3.14159265358979323846;

std::vector<float> FindDominantFrequencies(const std::vector<float>& signal, int sample_rate) {
    int n = signal.size();
    std::vector<std::complex<double>> fft(n);
    
    // Simple DFT
    for (int k = 0; k < n / 2; ++k) {
        std::complex<double> sum(0, 0);
        for (int t = 0; t < n; ++t) {
            double angle = -2.0 * kPi * k * t / n;
            sum += static_cast<double>(signal[t]) * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        fft[k] = sum;
    }
    
    // Find peaks
    std::vector<std::pair<float, float>> magnitude_freq;
    for (int k = 1; k < n / 2 - 1; ++k) {
        float mag = std::abs(fft[k]);
        float freq = static_cast<float>(k) * sample_rate / n;
        if (mag > 50.0f) {
            magnitude_freq.push_back({mag, freq});
        }
    }
    
    // Sort by magnitude
    std::sort(magnitude_freq.begin(), magnitude_freq.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::vector<float> result;
    std::cout << "\nDetected frequencies:\n";
    for (size_t i = 0; i < std::min(size_t(10), magnitude_freq.size()); ++i) {
        std::cout << "  " << magnitude_freq[i].second << " Hz (magnitude: " << magnitude_freq[i].first << ")\n";
        result.push_back(magnitude_freq[i].second);
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

int main() {
    int sample_rate = 48000;
    int num_samples = 4800; // 100ms
    
    std::cout << "Testing String Resonator with 2 harmonics at 440 Hz:\n";
    std::cout << "Expected: 440 Hz and 880 Hz\n";
    
    std::vector<double> phases1;
    auto output = audio_nodes::GenerateStringResonator(440.0f, 2, 0.5f, num_samples, sample_rate, phases1);
    auto freqs = FindDominantFrequencies(output, sample_rate);
    
    if (freqs.size() >= 2) {
        std::cout << "\n✓ Test PASSED: Found " << freqs.size() << " harmonics\n";
        std::cout << "  Fundamental: " << freqs[0] << " Hz (expected ~440)\n";
        std::cout << "  2nd harmonic: " << freqs[1] << " Hz (expected ~880)\n";
    } else {
        std::cout << "\n✗ Test FAILED: Only found " << freqs.size() << " harmonics\n";
        return 1;
    }
    
    std::cout << "\n\nTesting String Resonator with 3 harmonics at 200 Hz:\n";
    std::cout << "Expected: 200 Hz, 400 Hz, and 600 Hz\n";
    
    std::vector<double> phases2;
    output = audio_nodes::GenerateStringResonator(200.0f, 3, 0.5f, num_samples, sample_rate, phases2);
    freqs = FindDominantFrequencies(output, sample_rate);
    
    if (freqs.size() >= 3) {
        std::cout << "\n✓ Test PASSED: Found " << freqs.size() << " harmonics\n";
        std::cout << "  Fundamental: " << freqs[0] << " Hz (expected ~200)\n";
        std::cout << "  2nd harmonic: " << freqs[1] << " Hz (expected ~400)\n";
        std::cout << "  3rd harmonic: " << freqs[2] << " Hz (expected ~600)\n";
    } else {
        std::cout << "\n✗ Test FAILED: Only found " << freqs.size() << " harmonics\n";
        return 1;
    }
    
    return 0;
}
