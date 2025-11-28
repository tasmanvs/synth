#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Standalone test for smoothed square wave generation
// This tests the algorithm without requiring the full node graph infrastructure

namespace {

// Generate smoothed square wave using the same algorithm as in audio_node_graph.cc
std::vector<float> GenerateSmoothedSquare(int num_samples, int sample_rate, 
                                          float frequency, float volume, 
                                          float smoothing_time_ms, double& phase) {
    std::vector<float> output(num_samples);
    const double pi = 3.14159265358979323846;
    double phase_increment = 2.0 * pi * frequency / sample_rate;
    
    // Calculate smoothing duration in radians  
    float smoothing_samples = (smoothing_time_ms / 1000.0f) * sample_rate;
    smoothing_samples = std::max(2.0f, smoothing_samples);
    double smoothing_phase = smoothing_samples * phase_increment;
    smoothing_phase = std::min(smoothing_phase, pi * 0.48); // Max 48% of half-period
    
    for (int i = 0; i < num_samples; i++) {
        // Normalize phase to [0, 2*pi)
        double norm_phase = std::fmod(phase, 2.0 * pi);
        if (norm_phase < 0) norm_phase += 2.0 * pi;
        
        float value;
        
        // The rising edge wraps around 0/2π, so we need to handle it specially
        // Check distance from phase=0 (accounting for wrap)
        double dist_from_zero = (norm_phase < pi) ? norm_phase : (2.0 * pi - norm_phase);
        
        if (dist_from_zero < smoothing_phase) {
            // Rising edge centered at phase=0 (wraps around 2π): -1 to +1
            // Map phase to transition parameter t ∈ [0, 1]
            double t;
            if (norm_phase <= smoothing_phase) {
                // First part: [0, smoothing_phase]
                t = 0.5 + norm_phase / (2.0 * smoothing_phase);
            } else {
                // Wrapped part: [2π - smoothing_phase, 2π]
                t = (norm_phase - (2.0 * pi - smoothing_phase)) / (2.0 * smoothing_phase);
            }
            t = std::max(0.0, std::min(1.0, t));
            float smooth_t = t * t * (3.0 - 2.0 * t);
            value = -1.0f + 2.0f * smooth_t;
        } else if (norm_phase < pi - smoothing_phase) {
            // High plateau
            value = 1.0f;
        } else if (norm_phase < pi + smoothing_phase) {
            // Falling edge centered at phase=π: +1 to -1
            double t = (norm_phase - (pi - smoothing_phase)) / (2.0 * smoothing_phase);
            t = std::max(0.0, std::min(1.0, t));
            float smooth_t = t * t * (3.0 - 2.0 * t);
            value = 1.0f - 2.0f * smooth_t;
        } else {
            // Low plateau
            value = -1.0f;
        }
        
        output[i] = volume * value;
        phase += phase_increment;
        
        // Wrap phase
        while (phase >= 2.0 * pi) {
            phase -= 2.0 * pi;
        }
        while (phase < 0.0) {
            phase += 2.0 * pi;
        }
    }
    
    return output;
}

class SmoothedSquareTest : public ::testing::Test {
protected:
    static constexpr int kSampleRate = 48000;
    static constexpr float kVolume = 0.5f;
};

TEST_F(SmoothedSquareTest, NoLargeDiscontinuities) {
    double phase = 0.0;
    const float frequency = 10.0f; // 10 Hz
    const float smoothing_ms = 1.0f;
    const int num_samples = kSampleRate; // 1 second
    
    auto samples = GenerateSmoothedSquare(num_samples, kSampleRate, frequency, 
                                         kVolume, smoothing_ms, phase);
    
    // Check for large jumps between consecutive samples
    float max_diff = 0.0f;
    int max_diff_index = 0;
    for (size_t i = 1; i < samples.size(); i++) {
        float diff = std::abs(samples[i] - samples[i-1]);
        if (diff > max_diff) {
            max_diff = diff;
            max_diff_index = i;
        }
    }
    
    // Debug output around the problem area
    if (max_diff_index > 0 && max_diff_index < samples.size()) {
        std::cout << "Samples around index " << max_diff_index << ":\n";
        for (int j = std::max(0, max_diff_index - 3); j <= std::min((int)samples.size() - 1, max_diff_index + 3); j++) {
            std::cout << "  [" << j << "] = " << samples[j] << "\n";
        }
    }
    
    // At 48kHz with 1ms smoothing, max diff should be around:
    // Range of 2.0 over 48 samples = 0.042 per sample
    std::cout << "Max difference between consecutive samples: " << max_diff 
              << " at index " << max_diff_index << std::endl;
    
    EXPECT_LT(max_diff, 0.1f) 
        << "Large discontinuity detected: " << max_diff 
        << " at sample " << max_diff_index;
}

TEST_F(SmoothedSquareTest, PhaseContinuityAcrossBuffers) {
    double phase = 0.0;
    const float frequency = 440.0f;
    const float smoothing_ms = 1.0f;
    const int buffer_size = 1024;
    
    std::vector<float> prev_buffer = GenerateSmoothedSquare(buffer_size, kSampleRate, 
                                                            frequency, kVolume, 
                                                            smoothing_ms, phase);
    
    // Generate 20 consecutive buffers and check boundaries
    for (int i = 0; i < 20; i++) {
        auto curr_buffer = GenerateSmoothedSquare(buffer_size, kSampleRate, 
                                                 frequency, kVolume, 
                                                 smoothing_ms, phase);
        
        float last_sample = prev_buffer[prev_buffer.size() - 1];
        float first_sample = curr_buffer[0];
        float boundary_diff = std::abs(first_sample - last_sample);
        
        EXPECT_LT(boundary_diff, 0.1f)
            << "Buffer boundary discontinuity at buffer " << i
            << ": " << boundary_diff;
        
        prev_buffer = curr_buffer;
    }
}

TEST_F(SmoothedSquareTest, AmplitudeCorrect) {
    double phase = 0.0;
    const float frequency = 10.0f;
    const float smoothing_ms = 1.0f;
    const int num_samples = kSampleRate; // 1 second
    
    auto samples = GenerateSmoothedSquare(num_samples, kSampleRate, frequency, 
                                         kVolume, smoothing_ms, phase);
    
    float min_val = *std::min_element(samples.begin(), samples.end());
    float max_val = *std::max_element(samples.begin(), samples.end());
    
    std::cout << "Min: " << min_val << ", Max: " << max_val << std::endl;
    
    // Should reach close to +/- volume
    EXPECT_NEAR(max_val, kVolume, 0.01f);
    EXPECT_NEAR(min_val, -kVolume, 0.01f);
}

TEST_F(SmoothedSquareTest, DerivativeIsBounded) {
    double phase = 0.0;
    const float frequency = 100.0f;
    const float smoothing_ms = 1.0f;
    const int num_samples = kSampleRate / 10; // 0.1 second
    
    auto samples = GenerateSmoothedSquare(num_samples, kSampleRate, frequency, 
                                         kVolume, smoothing_ms, phase);
    
    // Compute maximum derivative
    float max_derivative = 0.0f;
    int max_deriv_index = 0;
    for (size_t i = 1; i < samples.size(); i++) {
        float derivative = std::abs(samples[i] - samples[i-1]) * kSampleRate;
        if (derivative > max_derivative) {
            max_derivative = derivative;
            max_deriv_index = i;
        }
    }
    
    std::cout << "Max derivative: " << max_derivative 
              << " at sample " << max_deriv_index << std::endl;
    
    // With 1ms smoothing over range of 2*volume (1.0), max derivative should be:
    // 1.0 / 0.001 = 1000
    // Allow some headroom
    EXPECT_LT(max_derivative, 2000.0f)
        << "Derivative too large: " << max_derivative;
}

TEST_F(SmoothedSquareTest, VisualInspection) {
    // Generate a CSV file for visual inspection
    double phase = 0.0;
    const float frequency = 10.0f;
    const float smoothing_ms = 1.0f;
    const int num_samples = kSampleRate / 5; // 0.2 seconds, should show 2 full cycles
    
    auto samples = GenerateSmoothedSquare(num_samples, kSampleRate, frequency, 
                                         kVolume, smoothing_ms, phase);
    
    std::ofstream csv_file("smoothed_square_output.csv");
    if (csv_file.is_open()) {
        csv_file << "sample,value\n";
        for (size_t i = 0; i < samples.size(); i++) {
            csv_file << i << "," << samples[i] << "\n";
        }
        csv_file.close();
        std::cout << "Generated smoothed_square_output.csv for visual inspection" << std::endl;
    }
    
    // This test always passes, it's just for generating output
    SUCCEED();
}

} // namespace
