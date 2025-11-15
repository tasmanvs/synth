#include "audio/audio_interface.h"
#include <stdexcept>

// Stub implementation of AudioInterface for testing
// Tests don't create PlayerNode, so these methods are never called

AudioInterface::AudioInterface() 
    : m_device(nullptr)
    , m_context(nullptr)
    , m_source(0)
    , m_buffer(0)
    , m_isPlaying(false) {
}

AudioInterface::~AudioInterface() {
}

bool AudioInterface::init() {
    return true;
}

void AudioInterface::playSamples(const std::vector<short>& samples, int sample_rate, bool looping) {
    throw std::runtime_error("AudioInterface::playSamples() called in test - tests should not use PlayerNode");
}

void AudioInterface::play() {
    throw std::runtime_error("AudioInterface::play() called in test - tests should not use PlayerNode");
}

void AudioInterface::stop() {
    throw std::runtime_error("AudioInterface::stop() called in test - tests should not use PlayerNode");
}

void AudioInterface::setVolume(float volume) {
    throw std::runtime_error("AudioInterface::setVolume() called in test - tests should not use PlayerNode");
}
