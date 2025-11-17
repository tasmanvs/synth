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
#include <memory>
#include <mutex>
#include <windows.h>
#include <xaudio2.h>

#include "absl/log/log.h"

namespace audio_loop {
namespace {
constexpr float kMaxSampleValue = 32767.0f;
}

struct AudioBufferPlayer::BufferContext {
    std::vector<int16_t> samples;
};

class AudioBufferPlayer::VoiceCallback : public IXAudio2VoiceCallback {
public:
    explicit VoiceCallback(AudioBufferPlayer* owner) : owner_(owner) {}

    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}

    void STDMETHODCALLTYPE OnBufferEnd(void* context) override {
        if (owner_ != nullptr) {
            owner_->HandleBufferEnd(context);
        }
    }

private:
    AudioBufferPlayer* owner_;
};

AudioBufferPlayer::AudioBufferPlayer()
        : initialized_(false),
            xaudio2_(nullptr),
            mastering_voice_(nullptr),
            source_voice_(nullptr),
            pending_buffers_(),
            pending_mutex_(),
            voice_callback_(std::make_unique<VoiceCallback>(this)),
            voice_running_(false),
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
    ClearPendingBuffers();
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
        voice_running_ = false;
        ClearPendingBuffers();
    }

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HRESULT hr = xaudio2_->CreateSourceVoice(&source_voice_, &wfx, 0,
                                             XAUDIO2_DEFAULT_FREQ_RATIO,
                                             voice_callback_.get());
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to create source voice: 0x" << std::hex << hr;
        source_voice_ = nullptr;
        current_sample_rate_ = 0;
        return false;
    }

    source_voice_->SetVolume(volume_);
    current_sample_rate_ = sample_rate;
    voice_running_ = false;
    return true;
}

bool AudioBufferPlayer::Play(const std::vector<float>& samples, int sample_rate) {
    Stop();
    return Append(samples, sample_rate, true);
}

bool AudioBufferPlayer::Append(const std::vector<float>& samples, int sample_rate, bool end_stream) {
    if (samples.empty()) {
        return false;
    }

    if (!InitializeSourceVoice(sample_rate)) {
        return false;
    }

    if (!SubmitBuffer(samples, end_stream)) {
        return false;
    }

    if (!voice_running_) {
        HRESULT hr = source_voice_->Start();
        if (FAILED(hr)) {
            LOG(ERROR) << "Failed to start source voice: 0x" << std::hex << hr;
            return false;
        }
        voice_running_ = true;
    }

    return true;
}

void AudioBufferPlayer::Stop() {
    if (source_voice_ != nullptr) {
        source_voice_->Stop();
        source_voice_->FlushSourceBuffers();
    }
    voice_running_ = false;
    ClearPendingBuffers();
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

int AudioBufferPlayer::GetQueuedBufferCount() const {
    if (source_voice_ == nullptr) {
        return 0;
    }
    XAUDIO2_VOICE_STATE state;
    source_voice_->GetState(&state);
    return static_cast<int>(state.BuffersQueued);
}

bool AudioBufferPlayer::SubmitBuffer(const std::vector<float>& samples,
                                     bool end_stream) {
    auto context = std::make_unique<BufferContext>();
    context->samples.resize(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        context->samples[i] = FloatToSample(samples[i]);
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(context->samples.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<BYTE*>(context->samples.data());
    buffer.pContext = context.get();
    if (end_stream) {
        buffer.Flags = XAUDIO2_END_OF_STREAM;
    }

    HRESULT hr = source_voice_->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to submit buffer: 0x" << std::hex << hr;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_buffers_.push_back(std::move(context));
    }

    return true;
}

void AudioBufferPlayer::HandleBufferEnd(void* context) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    auto it = std::find_if(pending_buffers_.begin(), pending_buffers_.end(),
                           [context](const std::unique_ptr<BufferContext>& pending) {
                               return pending.get() == context;
                           });
    if (it != pending_buffers_.end()) {
        pending_buffers_.erase(it);
    }
}

void AudioBufferPlayer::ClearPendingBuffers() {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    pending_buffers_.clear();
}

int16_t AudioBufferPlayer::FloatToSample(float value) {
    const float clamped = std::max(-1.0f, std::min(1.0f, value));
    return static_cast<int16_t>(clamped * kMaxSampleValue);
}

}  // namespace audio_loop
