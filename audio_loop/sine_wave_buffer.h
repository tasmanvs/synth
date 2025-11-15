#pragma once

#include <vector>

namespace audio_loop {

struct AudioBuffer {
  AudioBuffer() = default;
  AudioBuffer(int sample_rate_in, int num_frames)
      : sample_rate(sample_rate_in), samples(num_frames, 0.0f) {}

  int sample_rate = 0;
  std::vector<float> samples;
};

class SineWaveBufferGenerator {
 public:
  SineWaveBufferGenerator(float frequency_hz, double initial_phase = 0.0);

  AudioBuffer CreateBuffer(int num_frames, int sample_rate);
  double GetPhase() const;

 private:
  float frequency_hz_;
  double phase_;
};

}  // namespace audio_loop
