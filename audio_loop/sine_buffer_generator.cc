#include "audio_loop/sine_buffer_generator.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace audio_loop {
namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;

double WrapPhase(double phase_radians) {
    if (phase_radians >= 0.0) {
        return std::fmod(phase_radians, kTwoPi);
    }
    const double wrapped = std::fmod(phase_radians, kTwoPi) + kTwoPi;
    return wrapped >= kTwoPi ? wrapped - kTwoPi : wrapped;
}

float ClampAmplitude(float amplitude) {
    if (amplitude < 0.0f) {
        return 0.0f;
    }
    if (amplitude > 1.0f) {
        return 1.0f;
    }
    return amplitude;
}
}  // namespace

PhaseContinuousSine::PhaseContinuousSine(float starting_phase_radians)
    : phase_radians_(WrapPhase(static_cast<double>(starting_phase_radians))) {}

std::vector<float> PhaseContinuousSine::GenerateBuffer(const BufferConfig& config) {
    if (config.sample_rate <= 0 || config.frame_count <= 0) {
        return {};
    }

    std::vector<float> samples(static_cast<size_t>(config.frame_count));
    const float amplitude = ClampAmplitude(config.amplitude);
    const double frequency_hz =
        std::max(static_cast<double>(config.frequency_hz), 0.0);
    const double phase_increment = (kTwoPi * frequency_hz) /
                                   static_cast<double>(config.sample_rate);

    for (int i = 0; i < config.frame_count; ++i) {
        samples[static_cast<size_t>(i)] =
            static_cast<float>(std::sin(phase_radians_) * amplitude);
        phase_radians_ += phase_increment;
        if (phase_radians_ >= kTwoPi) {
            phase_radians_ = std::fmod(phase_radians_, kTwoPi);
        } else if (phase_radians_ < 0.0) {
            phase_radians_ = std::fmod(phase_radians_, kTwoPi) + kTwoPi;
        }
    }

    return samples;
}

void PhaseContinuousSine::ResetPhase(float phase_radians) {
    phase_radians_ = WrapPhase(static_cast<double>(phase_radians));
}

float PhaseContinuousSine::CurrentPhase() const {
    return static_cast<float>(phase_radians_);
}

ContinuityResult EvaluateContinuity(const std::vector<float>& previous,
                                    const std::vector<float>& current,
                                    float tolerance) {
    ContinuityResult result;
    if (previous.empty() || current.empty()) {
        return result;
    }

    const float diff = std::abs(current.front() - previous.back());
    result.difference = diff;
    result.is_continuous = diff <= tolerance;
    return result;
}

std::vector<float> ConcatenateBuffers(const std::vector<std::vector<float>>& buffers) {
    size_t total_size = 0;
    for (const auto& buffer : buffers) {
        total_size += buffer.size();
    }

    std::vector<float> combined;
    combined.reserve(total_size);
    for (const auto& buffer : buffers) {
        combined.insert(combined.end(), buffer.begin(), buffer.end());
    }
    return combined;
}

}  // namespace audio_loop
