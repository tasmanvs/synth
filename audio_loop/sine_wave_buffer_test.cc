#include "audio_loop/sine_wave_buffer.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
}

namespace audio_loop {
namespace {

TEST(SineWaveBufferGeneratorTest, GeneratesRequestedLength) {
  SineWaveBufferGenerator generator(440.0f);
  AudioBuffer buffer = generator.CreateBuffer(256, 44100);

  EXPECT_EQ(buffer.samples.size(), 256);
  EXPECT_EQ(buffer.sample_rate, 44100);
}

TEST(SineWaveBufferGeneratorTest, ProducesSineSamples) {
  SineWaveBufferGenerator generator(440.0f);
  constexpr int kSampleRate = 48000;
  constexpr int kNumFrames = 2;
  AudioBuffer buffer = generator.CreateBuffer(kNumFrames, kSampleRate);

  double expected_phase = 0.0;
  const double phase_increment = kTwoPi * 440.0 / kSampleRate;
  for (int i = 0; i < kNumFrames; ++i) {
    EXPECT_NEAR(buffer.samples[i], std::sin(expected_phase), 1e-5)
        << "Mismatch at sample " << i;
    expected_phase += phase_increment;
  }
}

TEST(SineWaveBufferGeneratorTest, MaintainsPhaseAcrossBuffers) {
  SineWaveBufferGenerator generator(440.0f);
  constexpr int kSampleRate = 44100;
  constexpr int kNumFrames = 128;

  AudioBuffer buffer_one = generator.CreateBuffer(kNumFrames, kSampleRate);
  AudioBuffer buffer_two = generator.CreateBuffer(kNumFrames, kSampleRate);

  const double phase_increment = kTwoPi * 440.0 / kSampleRate;
  const float expected_next_sample =
      static_cast<float>(std::sin(phase_increment * kNumFrames));
  EXPECT_NEAR(buffer_two.samples[0], expected_next_sample, 1e-4f);

  const double final_phase = generator.GetPhase();
  const double total_samples = static_cast<double>(kNumFrames * 2);
  const double expected_phase = std::fmod(total_samples * phase_increment, kTwoPi);
  EXPECT_NEAR(final_phase, expected_phase, 1e-5);
}

TEST(SineWaveBufferGeneratorTest, HandlesDifferentSampleRates) {
  constexpr float kFrequency = 220.0f;
  SineWaveBufferGenerator generator_low(kFrequency);
  SineWaveBufferGenerator generator_high(kFrequency);

  AudioBuffer buffer_low = generator_low.CreateBuffer(2, 22050);
  AudioBuffer buffer_high = generator_high.CreateBuffer(2, 48000);

  EXPECT_EQ(buffer_low.sample_rate, 22050);
  EXPECT_EQ(buffer_high.sample_rate, 48000);
  ASSERT_EQ(buffer_low.samples.size(), 2);
  ASSERT_EQ(buffer_high.samples.size(), 2);

  const double expected_low_second = std::sin(kTwoPi * kFrequency / 22050.0);
  const double expected_high_second = std::sin(kTwoPi * kFrequency / 48000.0);
  EXPECT_NEAR(buffer_low.samples[1], expected_low_second, 1e-5);
  EXPECT_NEAR(buffer_high.samples[1], expected_high_second, 1e-5);
}

}  // namespace
}  // namespace audio_loop
