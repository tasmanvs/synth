#include "audio/nodes/source_node.h"
#include "gtest/gtest.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include <iostream>

namespace audio_nodes {
namespace {

const double kPi = 3.14159265358979323846;
const float kTolerance = 0.01f;

// Helper to perform FFT and find dominant frequencies
std::vector<float> FindDominantFrequencies(const std::vector<float>& signal, int sample_rate, int top_n = 5, bool debug = false) {
    int n = signal.size();
    std::vector<std::complex<double>> fft(n);
    
    // Simple DFT for testing
    for (int k = 0; k < n / 2; ++k) {
        std::complex<double> sum(0, 0);
        for (int t = 0; t < n; ++t) {
            double angle = -2.0 * kPi * k * t / n;
            sum += signal[t] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        fft[k] = sum;
    }
    
    // Find peaks
    std::vector<std::pair<float, float>> magnitude_freq;
    for (int k = 1; k < n / 2 - 1; ++k) {
        float mag = std::abs(fft[k]);
        float freq = static_cast<float>(k) * sample_rate / n;
        magnitude_freq.push_back({mag, freq});
    }
    
    // Sort by magnitude
    std::sort(magnitude_freq.begin(), magnitude_freq.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::vector<float> result;
    for (int i = 0; i < std::min(top_n, static_cast<int>(magnitude_freq.size())); ++i) {
        if (magnitude_freq[i].first > 10.0f) { // Threshold to avoid noise
            result.push_back(magnitude_freq[i].second);
            if (debug) {
                std::cout << "  Frequency: " << magnitude_freq[i].second << " Hz, Magnitude: " << magnitude_freq[i].first << std::endl;
            }
        }
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

TEST(SourceNodeTest, StringResonatorGeneratesCorrectHarmonics) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kStringResonator);
    node.SetFrequency(440.0f);
    node.SetVolume(0.5f);
    node.SetNumHarmonics(2);
    
    int sample_rate = 48000;
    int num_samples = 4800; // 100ms at 48kHz
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    
    // Find dominant frequencies
    auto freqs = FindDominantFrequencies(output, sample_rate, 3);
    
    ASSERT_GE(freqs.size(), 2) << "Should detect at least 2 harmonics";
    
    // Check for fundamental (440 Hz) and first harmonic (880 Hz)
    EXPECT_NEAR(freqs[0], 440.0f, 20.0f) << "First frequency should be ~440 Hz";
    EXPECT_NEAR(freqs[1], 880.0f, 20.0f) << "Second frequency should be ~880 Hz";
}

TEST(SourceNodeTest, StringResonatorWithThreeHarmonics) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kStringResonator);
    node.SetFrequency(200.0f);
    node.SetVolume(0.5f);
    node.SetNumHarmonics(3);
    
    int sample_rate = 48000;
    int num_samples = 4800;
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    auto freqs = FindDominantFrequencies(output, sample_rate, 4);
    
    ASSERT_GE(freqs.size(), 3);
    
    // Check for 200 Hz, 400 Hz, 600 Hz
    EXPECT_NEAR(freqs[0], 200.0f, 15.0f);
    EXPECT_NEAR(freqs[1], 400.0f, 15.0f);
    EXPECT_NEAR(freqs[2], 600.0f, 15.0f);
}

TEST(SourceNodeTest, SineWaveGeneratesSingleFrequency) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kSine);
    node.SetFrequency(1000.0f);
    node.SetVolume(0.5f);
    
    int sample_rate = 48000;
    int num_samples = 4800;
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    auto freqs = FindDominantFrequencies(output, sample_rate, 2);
    
    ASSERT_GE(freqs.size(), 1);
    EXPECT_NEAR(freqs[0], 1000.0f, 20.0f);
}

TEST(SourceNodeTest, MultipleFrequenciesInclusive) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kSine);
    node.SetFrequency(200.0f);
    node.SetFrequencyEnd(400.0f);
    node.SetFrequencyCount(3);
    node.SetEndFrequencyInclusive(true);
    node.SetVolume(0.5f);
    
    int sample_rate = 48000;
    int num_samples = 4800;
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    auto freqs = FindDominantFrequencies(output, sample_rate, 4);
    
    ASSERT_GE(freqs.size(), 3);
    
    // With 3 frequencies inclusive: 200, 300, 400
    EXPECT_NEAR(freqs[0], 200.0f, 15.0f);
    EXPECT_NEAR(freqs[1], 300.0f, 15.0f);
    EXPECT_NEAR(freqs[2], 400.0f, 15.0f);
}

TEST(SourceNodeTest, MultipleFrequenciesExclusive) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kSine);
    node.SetFrequency(200.0f);
    node.SetFrequencyEnd(400.0f);
    node.SetFrequencyCount(2);
    node.SetEndFrequencyInclusive(false);
    node.SetVolume(0.5f);
    
    int sample_rate = 48000;
    int num_samples = 4800;
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    auto freqs = FindDominantFrequencies(output, sample_rate, 3);
    
    ASSERT_GE(freqs.size(), 2);
    
    // With 2 frequencies exclusive: 200, 300 (not 400)
    EXPECT_NEAR(freqs[0], 200.0f, 15.0f);
    EXPECT_NEAR(freqs[1], 300.0f, 15.0f);
}

TEST(SourceNodeTest, ZeroVolumeProducesSilence) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kSine);
    node.SetFrequency(440.0f);
    node.SetVolume(0.0f);
    
    int sample_rate = 48000;
    int num_samples = 100;
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    
    for (float sample : output) {
        EXPECT_EQ(sample, 0.0f);
    }
}

TEST(SourceNodeTest, ZeroFrequencyProducesSilence) {
    SourceNode node(1);
    node.SetWaveformType(WaveformType::kSine);
    node.SetFrequency(0.0f);
    node.SetVolume(0.5f);
    
    int sample_rate = 48000;
    int num_samples = 100;
    
    std::vector<float> output = node.GenerateAudio(num_samples, sample_rate);
    
    for (float sample : output) {
        EXPECT_EQ(sample, 0.0f);
    }
}

} // namespace
} // namespace audio_nodes
