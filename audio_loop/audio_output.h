#pragma once

#include <cstdint>
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
    void Stop();
    void SetVolume(float volume);
    bool IsPlaying() const;

private:
    bool InitializeSourceVoice(int sample_rate);
    static int16_t FloatToSample(float value);

    bool initialized_;
    IXAudio2* xaudio2_;
    IXAudio2MasteringVoice* mastering_voice_;
    IXAudio2SourceVoice* source_voice_;
    std::vector<int16_t> last_samples_;
    float volume_;
    int current_sample_rate_;
};

}  // namespace audio_loop
