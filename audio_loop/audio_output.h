#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;

namespace audio_loop {

class AudioBufferPlayer {
public:
    AudioBufferPlayer();
    ~AudioBufferPlayer();

    bool Initialize();
    void Shutdown();

    bool Play(const std::vector<float>& samples, int sample_rate);
    bool Append(const std::vector<float>& samples, int sample_rate, bool end_stream = false);
    void Stop();
    void SetVolume(float volume);
    bool IsPlaying() const;
    int GetQueuedBufferCount() const;

private:
    struct BufferContext;
    class VoiceCallback;

    bool InitializeSourceVoice(int sample_rate);
    bool SubmitBuffer(const std::vector<float>& samples, bool end_stream);
    void HandleBufferEnd(void* context);
    void ClearPendingBuffers();
    static int16_t FloatToSample(float value);

    bool initialized_;
    IXAudio2* xaudio2_;
    IXAudio2MasteringVoice* mastering_voice_;
    IXAudio2SourceVoice* source_voice_;
    std::deque<std::unique_ptr<BufferContext>> pending_buffers_;
    mutable std::mutex pending_mutex_;
    std::unique_ptr<VoiceCallback> voice_callback_;
    bool voice_running_;
    float volume_;
    int current_sample_rate_;
};

}  // namespace audio_loop
