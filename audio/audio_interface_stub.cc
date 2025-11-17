#include "audio/audio_interface.h"
#include <stdexcept>

// Stub implementation of AudioInterface for testing
// Tests don't create PlayerNode, so these methods are never called

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
}

bool AudioInterface::Init() {
    return true;
}

void AudioInterface::PlaySamples(const std::vector<short>& /*samples*/, int /*sample_rate*/, bool /*looping*/) {
    throw std::runtime_error("AudioInterface::PlaySamples() called in test - tests should not use PlayerNode");
}

void AudioInterface::Play() {
    throw std::runtime_error("AudioInterface::Play() called in test - tests should not use PlayerNode");
}

void AudioInterface::Stop() {
    throw std::runtime_error("AudioInterface::Stop() called in test - tests should not use PlayerNode");
}

void AudioInterface::SetVolume(float /*volume*/) {
    throw std::runtime_error("AudioInterface::SetVolume() called in test - tests should not use PlayerNode");
}

bool AudioInterface::AppendSamples(const std::vector<short>& /*samples*/, int /*sample_rate*/, bool /*end_stream*/) {
    throw std::runtime_error("AudioInterface::AppendSamples() called in test - tests should not use PlayerNode");
}

void AudioInterface::ServiceStreamingQueue() {
}

void AudioInterface::ClearStreamingQueue() {
}

int AudioInterface::GetQueuedBufferCount() const {
    return 0;
}

bool AudioInterface::QueueStreamingBuffer(const std::vector<short>& /*samples*/, int /*sample_rate*/) {
    return false;
}

void AudioInterface::UnqueueProcessedBuffers() {
}

void AudioInterface::DeleteAllQueuedBuffers() {
}
