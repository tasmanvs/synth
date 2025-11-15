#include "audio_loop/sine_wave_buffer.h"

#include <cmath>

namespace {
constexpr double kTwoPi = 6.28318530717958647692;

double WrapPhase(double phase) {
  if (phase >= kTwoPi || phase < 0.0) {
    phase = std::fmod(phase, kTwoPi);
    if (phase < 0.0) {
      phase += kTwoPi;
    }
  }
  return phase;
}
}  // namespace

namespace audio_loop {

SineWaveBufferGenerator::SineWaveBufferGenerator(float frequency_hz,
                                                 double initial_phase)
    : frequency_hz_(frequency_hz), phase_(WrapPhase(initial_phase)) {}

double SineWaveBufferGenerator::GetPhase() const { return phase_; }

AudioBuffer SineWaveBufferGenerator::CreateBuffer(int num_frames,
                                                  int sample_rate) {
  if (num_frames < 0) {
    num_frames = 0;
  }
  AudioBuffer buffer(sample_rate, num_frames);
  if (sample_rate <= 0 || num_frames == 0) {
    return buffer;
  }

  const double phase_increment =
      kTwoPi * static_cast<double>(frequency_hz_) /
      static_cast<double>(sample_rate);
  for (int i = 0; i < num_frames; ++i) {
    buffer.samples[i] = static_cast<float>(std::sin(phase_));
    phase_ += phase_increment;
    phase_ = WrapPhase(phase_);
  }

  return buffer;
}

}  // namespace audio_loop
