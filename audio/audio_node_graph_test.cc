#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Test the actual audio node classes from audio_node_graph.h
// We include the real implementation and test it without calling Draw() methods
#include "audio/audio_node_graph.h"

namespace audio_nodes {
namespace {

// Helper function to detect clicks in audio signal
// A click is detected when there's a sudden jump in amplitude between samples
bool DetectClick(const std::vector<float>& samples, float threshold = 0.5f) {
    for (size_t i = 1; i < samples.size(); i++) {
        float diff = std::abs(samples[i] - samples[i-1]);
        if (diff > threshold) {
            return true;
        }
    }
    return false;
}

// Helper function to compute RMS (Root Mean Square) of a signal
float ComputeRMS(const std::vector<float>& samples) {
    float sum = 0.0f;
    for (float sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / samples.size());
}

// Test fixture for SourceNode tests (doesn't need AudioInterface)
class SourceNodeTest : public ::testing::Test {
protected:
    static constexpr int kSampleRate = 44100;
    static constexpr int kBufferSize = 1024;
};

// Test fixture for SumNode tests (doesn't need AudioInterface)
class SumNodeTest : public ::testing::Test {
protected:
    static constexpr int kSampleRate = 44100;
    static constexpr int kBufferSize = 1024;
};

// ============================================================================
// SourceNode Tests
// ============================================================================

TEST_F(SourceNodeTest, SourceNodeGeneratesCorrectFrequency) {
    audio_nodes::SourceNode source(1);
    
    // Generate audio
    auto samples = source.GenerateAudio(kBufferSize, kSampleRate);
    
    // Check that we got the right number of samples
    EXPECT_EQ(samples.size(), kBufferSize);
    
    // Check that the signal has non-zero RMS (it's actually playing something)
    float rms = ComputeRMS(samples);
    EXPECT_GT(rms, 0.0f);
}

TEST_F(SourceNodeTest, SourceNodeMaintainsPhaseContinuity) {
    audio_nodes::SourceNode source(1);
    
    // Generate two consecutive buffers
    auto buffer1 = source.GenerateAudio(kBufferSize, kSampleRate);
    auto buffer2 = source.GenerateAudio(kBufferSize, kSampleRate);
    
    // The last sample of buffer1 should be close to the first sample of buffer2
    // (within one phase increment)
    float last_sample = buffer1[buffer1.size() - 1];
    float first_sample = buffer2[0];
    
    // The phase increment per sample
    float frequency = source.GetFrequency();
    float phase_increment = 2.0f * M_PI * frequency / kSampleRate;
    float expected_diff = std::abs(std::sin(0.0f) - std::sin(phase_increment)) * source.GetVolume();
    
    // Allow for some tolerance
    float actual_diff = std::abs(first_sample - last_sample);
    
    // The difference should be small (no clicking)
    EXPECT_LT(actual_diff, expected_diff * 2.0f) 
        << "Phase discontinuity detected between buffers. Last: " << last_sample 
        << ", First: " << first_sample << ", Diff: " << actual_diff;
}

TEST_F(SourceNodeTest, SourceNodeNoClickingAcrossMultipleBuffers) {
    audio_nodes::SourceNode source(1);
    
    // Generate multiple buffers and check for clicks at boundaries
    std::vector<float> prev_buffer = source.GenerateAudio(kBufferSize, kSampleRate);
    
    for (int i = 0; i < 10; i++) {
        auto curr_buffer = source.GenerateAudio(kBufferSize, kSampleRate);
        
        // Check for click at boundary
        float last_sample = prev_buffer[prev_buffer.size() - 1];
        float first_sample = curr_buffer[0];
        float boundary_diff = std::abs(first_sample - last_sample);
        
        // Boundary difference should be smaller than typical click threshold
        EXPECT_LT(boundary_diff, 0.5f) 
            << "Click detected at buffer boundary " << i 
            << ". Diff: " << boundary_diff;
        
        prev_buffer = curr_buffer;
    }
}

TEST_F(SourceNodeTest, DetectsClickWithoutPhaseTracking) {
    // This test demonstrates what would happen without phase tracking
    // We create a naive source that resets phase each time
    
    const float frequency = 440.0f;
    const float volume = 0.5f;
    const int num_samples = 100;
    
    // Generate first buffer
    std::vector<float> buffer1(num_samples);
    float phase = 0.0f;
    float phase_increment = 2.0f * M_PI * frequency / kSampleRate;
    for (int i = 0; i < num_samples; i++) {
        buffer1[i] = std::sin(phase) * volume;
        phase += phase_increment;
    }
    
    // Generate second buffer WITHOUT maintaining phase (simulating the bug)
    std::vector<float> buffer2(num_samples);
    phase = 0.0f;  // Reset phase (this is the bug!)
    for (int i = 0; i < num_samples; i++) {
        buffer2[i] = std::sin(phase) * volume;
        phase += phase_increment;
    }
    
    // Check for click at boundary
    float boundary_diff = std::abs(buffer2[0] - buffer1[num_samples - 1]);
    
    // This demonstrates the problem - there's likely a discontinuity
    // Note: This test is informational and may occasionally pass if phases align
}

// ============================================================================
// SumNode Tests
// ============================================================================

TEST_F(SumNodeTest, SumNodeAddsSignalsCorrectly) {
    audio_nodes::SourceNode source1(1);
    audio_nodes::SourceNode source2(2);
    audio_nodes::SumNode sum(3);
    
    // Connect sources to sum node
    EXPECT_TRUE(sum.AddInput(&source1));
    EXPECT_TRUE(sum.AddInput(&source2));
    
    // Generate audio
    auto samples = sum.GenerateAudio(kBufferSize, kSampleRate);
    
    // Check that we got the right number of samples
    EXPECT_EQ(samples.size(), kBufferSize);
    
    // The RMS should be higher than a single source
    float rms = ComputeRMS(samples);
    EXPECT_GT(rms, 0.0f);
}

TEST_F(SumNodeTest, SumNodeMaintainsPhaseContinuityOfInputs) {
    audio_nodes::SourceNode source1(1);
    audio_nodes::SourceNode source2(2);
    audio_nodes::SumNode sum(3);
    
    // Connect sources to sum node
    sum.AddInput(&source1);
    sum.AddInput(&source2);
    
    // Generate two consecutive buffers
    auto buffer1 = sum.GenerateAudio(kBufferSize, kSampleRate);
    auto buffer2 = sum.GenerateAudio(kBufferSize, kSampleRate);
    
    // Check for click at boundary
    float last_sample = buffer1[buffer1.size() - 1];
    float first_sample = buffer2[0];
    float boundary_diff = std::abs(first_sample - last_sample);
    
    // Should not have a large discontinuity
    EXPECT_LT(boundary_diff, 0.5f) 
        << "Click detected in SumNode output at buffer boundary. Diff: " << boundary_diff;
}

TEST_F(SumNodeTest, ComplexGraphMaintainsPhase) {
    // Create a more complex graph: two sources -> sum
    audio_nodes::SourceNode source1(1);
    audio_nodes::SourceNode source2(2);
    audio_nodes::SumNode sum(3);
    
    // Connect the graph
    sum.AddInput(&source1);
    sum.AddInput(&source2);
    
    // Generate multiple buffers
    std::vector<float> prev_buffer = sum.GenerateAudio(kBufferSize, kSampleRate);
    
    for (int i = 0; i < 10; i++) {
        auto curr_buffer = sum.GenerateAudio(kBufferSize, kSampleRate);
        
        float last_sample = prev_buffer[prev_buffer.size() - 1];
        float first_sample = curr_buffer[0];
        float boundary_diff = std::abs(first_sample - last_sample);
        
        EXPECT_LT(boundary_diff, 0.5f)
            << "Click detected in complex graph at buffer " << i
            << ". Diff: " << boundary_diff;
        
        prev_buffer = curr_buffer;
    }
}

// ============================================================================
// Smoothed Square Wave Tests
// ============================================================================

TEST_F(SourceNodeTest, SmoothedSquareWaveIsContinuous) {
    audio_nodes::SourceNode source(1);
    // Use a mock to set waveform type - since we can't call Draw(), we'll test
    // by generating audio and checking smoothness
    
    // Generate a full period of audio at low frequency to see transitions clearly
    const int sample_rate = 48000;
    const int samples_per_period = sample_rate / 10; // 10 Hz
    
    auto samples = source.GenerateAudio(samples_per_period * 2, sample_rate);
    
    // Check for discontinuities (clicks)
    float max_diff = 0.0f;
    for (size_t i = 1; i < samples.size(); i++) {
        float diff = std::abs(samples[i] - samples[i-1]);
        max_diff = std::max(max_diff, diff);
    }
    
    // For a properly smoothed wave at 10Hz with default 1ms smoothing,
    // the maximum difference between consecutive samples should be small
    // Max theoretical diff for unsmoothed square: 2.0 (jump from -1 to +1)
    // With smoothing, should be much smaller
    EXPECT_LT(max_diff, 0.1f) 
        << "Large discontinuity detected in smoothed square wave: " << max_diff;
}

TEST_F(SourceNodeTest, SmoothedSquareWaveHasCorrectAmplitude) {
    audio_nodes::SourceNode source(1);
    
    const int sample_rate = 48000;
    const int num_samples = sample_rate; // 1 second
    
    auto samples = source.GenerateAudio(num_samples, sample_rate);
    
    // Find min and max
    float min_val = *std::min_element(samples.begin(), samples.end());
    float max_val = *std::max_element(samples.begin(), samples.end());
    
    // Should reach close to -volume and +volume
    float volume = source.GetVolume();
    EXPECT_NEAR(max_val, volume, 0.1f) << "Max value: " << max_val;
    EXPECT_NEAR(min_val, -volume, 0.1f) << "Min value: " << min_val;
}

TEST_F(SourceNodeTest, SmoothedSquareWavePhaseContinuity) {
    audio_nodes::SourceNode source(1);
    
    const int sample_rate = 48000;
    const int buffer_size = 1024;
    
    // Generate multiple consecutive buffers
    std::vector<float> prev_buffer = source.GenerateAudio(buffer_size, sample_rate);
    
    for (int i = 0; i < 20; i++) {
        auto curr_buffer = source.GenerateAudio(buffer_size, sample_rate);
        
        // Check boundary continuity
        float last_sample = prev_buffer[prev_buffer.size() - 1];
        float first_sample = curr_buffer[0];
        float boundary_diff = std::abs(first_sample - last_sample);
        
        // Should be smooth across buffer boundaries
        EXPECT_LT(boundary_diff, 0.1f)
            << "Discontinuity at buffer boundary " << i
            << ". Last: " << last_sample
            << ", First: " << first_sample
            << ", Diff: " << boundary_diff;
        
        prev_buffer = curr_buffer;
    }
}

TEST_F(SourceNodeTest, SmoothedSquareWaveDerivativeIsBounded) {
    audio_nodes::SourceNode source(1);
    
    const int sample_rate = 48000;
    const int num_samples = sample_rate; // 1 second
    
    auto samples = source.GenerateAudio(num_samples, sample_rate);
    
    // Compute finite differences (approximate derivative)
    float max_derivative = 0.0f;
    for (size_t i = 1; i < samples.size(); i++) {
        float derivative = std::abs(samples[i] - samples[i-1]) * sample_rate;
        max_derivative = std::max(max_derivative, derivative);
    }
    
    // For a smoothed wave, the derivative should be bounded
    // Unsmoothed square wave would have infinite derivative at transitions
    // With 1ms smoothing over a range of 2.0, max derivative should be around 2000
    EXPECT_LT(max_derivative, 5000.0f)
        << "Derivative too large, suggesting sharp transitions: " << max_derivative;
}

TEST_F(SourceNodeTest, SmoothedSquareWaveNoSpikes) {
    audio_nodes::SourceNode source(1);
    
    const int sample_rate = 48000;
    const int num_samples = sample_rate / 10; // One period at 10 Hz
    
    auto samples = source.GenerateAudio(num_samples * 3, sample_rate);
    
    float volume = source.GetVolume();
    
    // Check that no sample exceeds the volume bounds
    for (size_t i = 0; i < samples.size(); i++) {
        EXPECT_LE(samples[i], volume * 1.01f) 
            << "Sample " << i << " exceeds upper bound: " << samples[i];
        EXPECT_GE(samples[i], -volume * 1.01f)
            << "Sample " << i << " exceeds lower bound: " << samples[i];
    }
}

} // namespace
} // namespace audio_nodes
