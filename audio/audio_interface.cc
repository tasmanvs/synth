#include "audio/audio_interface.h"
#include "absl/log/log.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <cstdio>

AudioInterface::AudioInterface()
    : device_(nullptr)
    , context_(nullptr)
    , source_(0)
    , buffer_(0)
    , is_playing_(false)
    , streaming_end_pending_(false)
    , streaming_sample_rate_(0) {
}

AudioInterface::~AudioInterface() {
    Stop();
    ClearStreamingQueue();

    if (buffer_) {
        alDeleteBuffers(1, &buffer_);
        buffer_ = 0;
    }
    if (source_) {
        alDeleteSources(1, &source_);
        source_ = 0;
    }
    if (context_) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(static_cast<ALCcontext*>(context_));
        context_ = nullptr;
    }
    if (device_) {
        alcCloseDevice(static_cast<ALCdevice*>(device_));
        device_ = nullptr;
    }

    LOG(INFO) << "AudioInterface destroyed";
}

bool AudioInterface::Init() {
    if (device_) {
        return true;  // Already initialized
    }

    LOG(INFO) << "Initializing AudioInterface";

    device_ = alcOpenDevice(nullptr);
    if (!device_) {
        LOG(ERROR) << "Failed to open audio device";
        return false;
    }

    context_ = alcCreateContext(static_cast<ALCdevice*>(device_), nullptr);
    if (!context_) {
        LOG(ERROR) << "Failed to create audio context";
        alcCloseDevice(static_cast<ALCdevice*>(device_));
        device_ = nullptr;
        return false;
    }

    if (!alcMakeContextCurrent(static_cast<ALCcontext*>(context_))) {
        LOG(ERROR) << "Failed to make context current";
        alcDestroyContext(static_cast<ALCcontext*>(context_));
        alcCloseDevice(static_cast<ALCdevice*>(device_));
        context_ = nullptr;
        device_ = nullptr;
        return false;
    }

    alGenSources(1, &source_);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to generate audio source: " << error;
        return false;
    }

    alSourcef(source_, AL_PITCH, 1.0f);
    alSourcef(source_, AL_GAIN, 0.3f);
    alSource3f(source_, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(source_, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

    LOG(INFO) << "AudioInterface initialized successfully";
    return true;
}

void AudioInterface::PlaySamples(const std::vector<short>& samples, int sample_rate, bool looping) {
    if (!Init()) {
        return;
    }

    current_samples_ = samples;
    ClearStreamingQueue();

    if (buffer_) {
        alSourcei(source_, AL_BUFFER, 0);
        alDeleteBuffers(1, &buffer_);
        buffer_ = 0;
    }

    alGenBuffers(1, &buffer_);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to generate audio buffer: " << error;
        return;
    }

    alBufferData(buffer_, AL_FORMAT_MONO16,
                 samples.data(), static_cast<ALsizei>(samples.size() * sizeof(short)),
                 sample_rate);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to fill audio buffer: " << error;
        return;
    }

    alSourcei(source_, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    alSourcei(source_, AL_BUFFER, buffer_);

    LOG(INFO) << "Loaded " << samples.size() << " samples at " << sample_rate << " Hz";
}

void AudioInterface::Play() {
    if (!is_playing_ && source_) {
        alSourcePlay(source_);
        is_playing_ = true;
        LOG(INFO) << "Audio playback started";
    }
}

void AudioInterface::Stop() {
    if (is_playing_ && source_) {
        alSourceStop(source_);
        is_playing_ = false;
        LOG(INFO) << "Audio playback stopped";
    }

    ClearStreamingQueue();
}

void AudioInterface::SetVolume(float volume) {
    if (source_) {
        alSourcef(source_, AL_GAIN, volume);
    }
}

bool AudioInterface::AppendSamples(const std::vector<short>& samples,
                                   int sample_rate,
                                   bool end_stream) {
    if (samples.empty() || !Init()) {
        return false;
    }

    ServiceStreamingQueue();

    if (streaming_sample_rate_ != 0 && streaming_sample_rate_ != sample_rate) {
        LOG(INFO) << "Sample rate changed from " << streaming_sample_rate_
                  << " to " << sample_rate << ", clearing streaming queue";
        ClearStreamingQueue();
    }

    if (!QueueStreamingBuffer(samples, sample_rate)) {
        return false;
    }

    streaming_sample_rate_ = sample_rate;
    current_samples_ = samples;

    if (!is_playing_ && source_) {
        alSourcePlay(source_);
        is_playing_ = true;
    }

    if (end_stream) {
        streaming_end_pending_ = true;
    }

    return true;
}

void AudioInterface::ServiceStreamingQueue() {
    if (!source_) {
        return;
    }

    UnqueueProcessedBuffers();

    if (queued_buffers_.empty()) {
        if (streaming_end_pending_) {
            streaming_end_pending_ = false;
            is_playing_ = false;
        }
        return;
    }

    ALint state = 0;
    alGetSourcei(source_, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source_);
        is_playing_ = true;
    }
}

void AudioInterface::ClearStreamingQueue() {
    if (!source_) {
        queued_buffers_.clear();
        streaming_sample_rate_ = 0;
        streaming_end_pending_ = false;
        return;
    }

    alSourceStop(source_);
    is_playing_ = false;
    DeleteAllQueuedBuffers();
    streaming_sample_rate_ = 0;
    streaming_end_pending_ = false;
}

int AudioInterface::GetQueuedBufferCount() const {
    return static_cast<int>(queued_buffers_.size());
}

bool AudioInterface::QueueStreamingBuffer(const std::vector<short>& samples, int sample_rate) {
    if (samples.empty() || !source_) {
        return false;
    }

    ALuint buffer_id = 0;
    alGenBuffers(1, &buffer_id);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to generate streaming buffer: " << error;
        return false;
    }

    alBufferData(buffer_id, AL_FORMAT_MONO16,
                 samples.data(), static_cast<ALsizei>(samples.size() * sizeof(short)),
                 sample_rate);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to fill streaming buffer: " << error;
        alDeleteBuffers(1, &buffer_id);
        return false;
    }

    alSourceQueueBuffers(source_, 1, &buffer_id);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to queue streaming buffer: " << error;
        alDeleteBuffers(1, &buffer_id);
        return false;
    }

    queued_buffers_.push_back(buffer_id);
    return true;
}

void AudioInterface::UnqueueProcessedBuffers() {
    if (!source_ || queued_buffers_.empty()) {
        return;
    }

    ALint processed = 0;
    alGetSourcei(source_, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0 && !queued_buffers_.empty()) {
        ALuint buffer_id = 0;
        alSourceUnqueueBuffers(source_, 1, &buffer_id);
        queued_buffers_.pop_front();
        if (buffer_id != 0) {
            alDeleteBuffers(1, &buffer_id);
        }
    }
}

void AudioInterface::DeleteAllQueuedBuffers() {
    if (!source_) {
        queued_buffers_.clear();
        return;
    }

    ALint queued = 0;
    alGetSourcei(source_, AL_BUFFERS_QUEUED, &queued);
    while (queued-- > 0) {
        ALuint buffer_id = 0;
        alSourceUnqueueBuffers(source_, 1, &buffer_id);
        if (buffer_id != 0) {
            alDeleteBuffers(1, &buffer_id);
        }
    }

    queued_buffers_.clear();
}
