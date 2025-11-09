#include "audio_synth.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <cmath>
#include <vector>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioSynth::AudioSynth() 
    : m_device(nullptr)
    , m_context(nullptr)
    , m_source(0)
    , m_buffer(0)
    , m_isPlaying(false) {
}

AudioSynth::~AudioSynth() {
    stopTone();
    
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
}

bool AudioSynth::init() {
    if (m_device) {
        return true; // Already initialized
    }
    
    // Open the default audio device
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        printf("Failed to open audio device\n");
        return false;
    }
    
    // Create audio context
    m_context = alcCreateContext(static_cast<ALCdevice*>(m_device), nullptr);
    if (!m_context) {
        printf("Failed to create audio context\n");
        alcCloseDevice(static_cast<ALCdevice*>(m_device));
        m_device = nullptr;
        return false;
    }
    
    // Make the context current
    if (!alcMakeContextCurrent(static_cast<ALCcontext*>(m_context))) {
        printf("Failed to make context current\n");
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
        printf("Failed to generate audio source: %d\n", error);
        return false;
    }
    
    // Set source properties
    alSourcef(m_source, AL_PITCH, 1.0f);
    alSourcef(m_source, AL_GAIN, 0.3f);
    alSource3f(m_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(m_source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcei(m_source, AL_LOOPING, AL_TRUE);
    
    return true;
}

void AudioSynth::generateSineWave(float frequency, float duration, int sampleRate) {
    // Calculate number of samples
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<short> samples(numSamples);
    
    // Generate sine wave
    for (int i = 0; i < numSamples; i++) {
        float t = static_cast<float>(i) / sampleRate;
        float value = std::sin(2.0f * M_PI * frequency * t);
        samples[i] = static_cast<short>(value * 32767.0f * 0.3f); // Scale and convert to 16-bit
    }
    
    // Delete old buffer if it exists
    if (m_buffer) {
        alDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
    
    // Generate new buffer
    alGenBuffers(1, &m_buffer);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        printf("Failed to generate audio buffer: %d\n", error);
        return;
    }
    
    // Fill buffer with sine wave data
    alBufferData(m_buffer, AL_FORMAT_MONO16, samples.data(), 
                 numSamples * sizeof(short), sampleRate);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        printf("Failed to fill audio buffer: %d\n", error);
        return;
    }
    
    // Attach buffer to source
    alSourcei(m_source, AL_BUFFER, m_buffer);
}

void AudioSynth::startTone(float frequency, float volume) {
    if (!init()) {
        return;
    }
    
    // Generate a 1-second sine wave at the given frequency
    // This will loop continuously due to AL_LOOPING being set to AL_TRUE
    generateSineWave(frequency, 1.0f, 44100);
    
    // Set volume
    alSourcef(m_source, AL_GAIN, volume);
    
    // Start playing if not already playing
    if (!m_isPlaying) {
        alSourcePlay(m_source);
        m_isPlaying = true;
    }
}

void AudioSynth::stopTone() {
    if (m_isPlaying && m_source) {
        alSourceStop(m_source);
        m_isPlaying = false;
    }
}

void AudioSynth::updateTone(float frequency, float volume) {
    if (!m_isPlaying) {
        return;
    }
    
    // Stop current playback
    alSourceStop(m_source);
    
    // Generate new sine wave with updated frequency
    generateSineWave(frequency, 1.0f, 44100);
    
    // Update volume
    alSourcef(m_source, AL_GAIN, volume);
    
    // Resume playback
    alSourcePlay(m_source);
}
