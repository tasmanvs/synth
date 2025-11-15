#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "audio_loop/sine_buffer_generator.h"

namespace audio_loop {
namespace {

TEST(PhaseContinuousSineTest, MaintainsContinuityBetweenBuffers) {
    BufferConfig config;
    config.frequency_hz = 523.25f;
    config.sample_rate = 48000;
    config.frame_count = 2048;
    config.amplitude = 0.7f;

    PhaseContinuousSine generator;
    const auto first = generator.GenerateBuffer(config);
    const auto second = generator.GenerateBuffer(config);

    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());

    const double phase_increment =
        6.2831853071795864769 * static_cast<double>(config.frequency_hz) /
        static_cast<double>(config.sample_rate);
    const float expected_tolerance = static_cast<float>(
        2.0 * config.amplitude * std::abs(std::sin(phase_increment / 2.0)) + 1e-4);
    const ContinuityResult result =
        EvaluateContinuity(first, second, expected_tolerance);
    EXPECT_TRUE(result.is_continuous);
}

TEST(PhaseContinuousSineTest, HandlesMultipleBufferSizes) {
    PhaseContinuousSine generator;
    BufferConfig config;
    config.frequency_hz = 440.0f;
    config.sample_rate = 44100;
    config.amplitude = 1.0f;

    const int buffer_sizes[] = {64, 1024, 4096};
    for (int size : buffer_sizes) {
        config.frame_count = size;
        const auto buffer = generator.GenerateBuffer(config);
        ASSERT_EQ(buffer.size(), static_cast<size_t>(size));
    }
}

TEST(PhaseContinuousSineTest, CanDetectDiscontinuity) {
    BufferConfig config;
    config.frequency_hz = 880.0f;
    config.sample_rate = 44100;
    config.frame_count = 512;

    PhaseContinuousSine continuous_generator;
    const auto first = continuous_generator.GenerateBuffer(config);
    const auto second = continuous_generator.GenerateBuffer(config);

    PhaseContinuousSine reset_generator;
    const auto discontinuous_second = reset_generator.GenerateBuffer(config);

    const ContinuityResult ok_result =
        EvaluateContinuity(first, second, 0.2f);
    EXPECT_TRUE(ok_result.is_continuous);

    const ContinuityResult bad_result =
        EvaluateContinuity(first, discontinuous_second, 0.2f);
    EXPECT_FALSE(bad_result.is_continuous);
}

TEST(PhaseContinuousSineTest, FrequencyChangesRemainSmooth) {
    BufferConfig config;
    config.frequency_hz = 220.0f;
    config.sample_rate = 48000;
    config.frame_count = 1024;
    config.amplitude = 0.9f;

    PhaseContinuousSine generator;
    const auto first = generator.GenerateBuffer(config);

    config.frequency_hz = 660.0f;
    const auto second = generator.GenerateBuffer(config);

    const ContinuityResult result =
        EvaluateContinuity(first, second, 0.15f);
    EXPECT_TRUE(result.is_continuous);
}

TEST(PhaseContinuousSineTest, ConcatenateBuffersProducesExpectedLength) {
    BufferConfig config;
    config.frequency_hz = 110.0f;
    config.sample_rate = 44100;
    config.frame_count = 128;

    PhaseContinuousSine generator;
    std::vector<std::vector<float>> buffers;
    for (int i = 0; i < 3; ++i) {
        buffers.push_back(generator.GenerateBuffer(config));
    }

    const auto combined = ConcatenateBuffers(buffers);
    EXPECT_EQ(combined.size(), buffers.size() * static_cast<size_t>(config.frame_count));
}

}  // namespace
}  // namespace audio_loop
