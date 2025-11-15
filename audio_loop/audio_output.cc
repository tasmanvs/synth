#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#ifdef WINVER
#undef WINVER
#endif
#define WINVER 0x0602
#define _WIN32_WINNT 0x0602

#include "audio_loop/audio_output.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <windows.h>
#include <xaudio2.h>

#include "absl/log/log.h"

namespace audio_loop {
namespace {
constexpr float kMaxSampleValue = 32767.0f;
}

AudioBufferPlayer::AudioBufferPlayer()
    : initialized_(false),
      xaudio2_(nullptr),
      mastering_voice_(nullptr),
      source_voice_(nullptr),
      volume_(0.6f),
      current_sample_rate_(0) {}

AudioBufferPlayer::~AudioBufferPlayer() {
    Shutdown();
}

bool AudioBufferPlayer::Initialize() {
    if (initialized_) {
        return true;
    }

    HRESULT hr = XAudio2Create(&xaudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to create XAudio2 engine: 0x" << std::hex << hr;
        return false;
    }

    hr = xaudio2_->CreateMasteringVoice(&mastering_voice_, 1, 0);
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to create mastering voice: 0x" << std::hex << hr;
        Shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

void AudioBufferPlayer::Shutdown() {
    if (source_voice_ != nullptr) {
        source_voice_->Stop();
        source_voice_->FlushSourceBuffers();
        source_voice_->DestroyVoice();
        source_voice_ = nullptr;
    }
    if (mastering_voice_ != nullptr) {
        mastering_voice_->DestroyVoice();
        mastering_voice_ = nullptr;
    }
    if (xaudio2_ != nullptr) {
        xaudio2_->Release();
        xaudio2_ = nullptr;
    }
    initialized_ = false;
    current_sample_rate_ = 0;
    last_samples_.clear();
}

bool AudioBufferPlayer::InitializeSourceVoice(int sample_rate) {
    if (!Initialize()) {
        return false;
    }

    if (source_voice_ != nullptr && current_sample_rate_ == sample_rate) {
        return true;
    }

    if (source_voice_ != nullptr) {
        source_voice_->Stop();
        source_voice_->FlushSourceBuffers();
        source_voice_->DestroyVoice();
        source_voice_ = nullptr;
    }

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HRESULT hr = xaudio2_->CreateSourceVoice(&source_voice_, &wfx);
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to create source voice: 0x" << std::hex << hr;
        source_voice_ = nullptr;
        current_sample_rate_ = 0;
        return false;
    }

    source_voice_->SetVolume(volume_);
    current_sample_rate_ = sample_rate;
    return true;
}

bool AudioBufferPlayer::Play(const std::vector<float>& samples, int sample_rate) {
    if (samples.empty()) {
        return false;
    }

    if (!InitializeSourceVoice(sample_rate)) {
        return false;
    }

    source_voice_->Stop();
    source_voice_->FlushSourceBuffers();

    last_samples_.resize(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        last_samples_[i] = FloatToSample(samples[i]);
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(last_samples_.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<BYTE*>(last_samples_.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    HRESULT hr = source_voice_->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to submit buffer: 0x" << std::hex << hr;
        return false;
    }

    hr = source_voice_->Start();
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to start source voice: 0x" << std::hex << hr;
        return false;
    }

    return true;
}

void AudioBufferPlayer::Stop() {
    if (source_voice_ != nullptr) {
        source_voice_->Stop();
        source_voice_->FlushSourceBuffers();
    }
}

void AudioBufferPlayer::SetVolume(float volume) {
    if (volume < 0.0f) {
        volume_ = 0.0f;
    } else if (volume > 1.0f) {
        volume_ = 1.0f;
    } else {
        volume_ = volume;
    }
    if (source_voice_ != nullptr) {
        source_voice_->SetVolume(volume_);
    }
}

bool AudioBufferPlayer::IsPlaying() const {
    if (source_voice_ == nullptr) {
        return false;
    }
    XAUDIO2_VOICE_STATE state;
    source_voice_->GetState(&state);
    return state.BuffersQueued > 0;
}

int16_t AudioBufferPlayer::FloatToSample(float value) {
    const float clamped = std::max(-1.0f, std::min(1.0f, value));
    return static_cast<int16_t>(clamped * kMaxSampleValue);
}

}  // namespace audio_loop
