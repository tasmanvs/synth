#include "audio_interface.h"
#include "absl/log/log.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <cstdio>

AudioInterface::AudioInterface() 
    : m_device(nullptr)
    , m_context(nullptr)
    , m_source(0)
    , m_buffer(0)
    , m_isPlaying(false) {
}

AudioInterface::~AudioInterface() {
    stop();
    
    if (m_buffer) {
        alDeleteBuffers(1, &m_buffer);
    }
    if (m_source) {
        alDeleteSources(1, &m_source);
    }
    if (m_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(static_cast<ALCcontext*>(m_context));
    }
    if (m_device) {
        alcCloseDevice(static_cast<ALCdevice*>(m_device));
    }
    
    LOG(INFO) << "AudioInterface destroyed";
}

bool AudioInterface::init() {
    if (m_device) {
        return true; // Already initialized
    }
    
    LOG(INFO) << "Initializing AudioInterface";
    
    // Open the default audio device
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        LOG(ERROR) << "Failed to open audio device";
        return false;
    }
    
    // Create audio context
    m_context = alcCreateContext(static_cast<ALCdevice*>(m_device), nullptr);
    if (!m_context) {
        LOG(ERROR) << "Failed to create audio context";
        alcCloseDevice(static_cast<ALCdevice*>(m_device));
        m_device = nullptr;
        return false;
    }
    
    // Make the context current
    if (!alcMakeContextCurrent(static_cast<ALCcontext*>(m_context))) {
        LOG(ERROR) << "Failed to make context current";
        alcDestroyContext(static_cast<ALCcontext*>(m_context));
        alcCloseDevice(static_cast<ALCdevice*>(m_device));
        m_context = nullptr;
        m_device = nullptr;
        return false;
    }
    
    // Generate source
    alGenSources(1, &m_source);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to generate audio source: " << error;
        return false;
    }
    
    // Set source properties
    alSourcef(m_source, AL_PITCH, 1.0f);
    alSourcef(m_source, AL_GAIN, 0.3f);
    alSource3f(m_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(m_source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    
    LOG(INFO) << "AudioInterface initialized successfully";
    return true;
}

void AudioInterface::playSamples(const std::vector<short>& samples, int sampleRate, bool looping) {
    if (!init()) {
        return;
    }
    
    // Store samples for visualization
    m_currentSamples = samples;
    
    // Delete old buffer if it exists
    if (m_buffer) {
        alSourcei(m_source, AL_BUFFER, 0); // Detach buffer first
        alDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
    
    // Generate new buffer
    alGenBuffers(1, &m_buffer);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to generate audio buffer: " << error;
        return;
    }
    
    // Fill buffer with sample data
    alBufferData(m_buffer, AL_FORMAT_MONO16, samples.data(), 
                 samples.size() * sizeof(short), sampleRate);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG(ERROR) << "Failed to fill audio buffer: " << error;
        return;
    }
    
    // Set looping
    alSourcei(m_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    
    // Attach buffer to source
    alSourcei(m_source, AL_BUFFER, m_buffer);
    
    LOG(INFO) << "Loaded " << samples.size() << " samples at " << sampleRate << " Hz";
}

void AudioInterface::play() {
    if (!m_isPlaying && m_source) {
        alSourcePlay(m_source);
        m_isPlaying = true;
        LOG(INFO) << "Audio playback started";
    }
}

void AudioInterface::stop() {
    if (m_isPlaying && m_source) {
        alSourceStop(m_source);
        m_isPlaying = false;
        LOG(INFO) << "Audio playback stopped";
    }
}

void AudioInterface::setVolume(float volume) {
    if (m_source) {
        alSourcef(m_source, AL_GAIN, volume);
    }
}
