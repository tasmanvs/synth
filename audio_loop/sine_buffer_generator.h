#pragma once

#include <cstddef>
#include <vector>

namespace audio_loop {

struct BufferConfig {
    float frequency_hz = 440.0f;
    float amplitude = 0.8f;
    int sample_rate = 48000;
    int frame_count = 512;
};

class PhaseContinuousSine {
public:
    explicit PhaseContinuousSine(float starting_phase_radians = 0.0f);

    std::vector<float> GenerateBuffer(const BufferConfig& config);

    void ResetPhase(float phase_radians = 0.0f);

    float CurrentPhase() const;

private:
    double phase_radians_;
};

struct ContinuityResult {
    bool is_continuous = false;
    float difference = 0.0f;
};

ContinuityResult EvaluateContinuity(const std::vector<float>& previous,
                                    const std::vector<float>& current,
                                    float tolerance = 1e-3f);

std::vector<float> ConcatenateBuffers(const std::vector<std::vector<float>>& buffers);

}  // namespace audio_loop
