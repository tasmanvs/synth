// Web Audio API for tone generation and custom buffer playback
var AudioSynth = {
    audioContext: null,
    oscillatorNode: null,
    bufferSourceNode: null,
    gainNode: null,
    isPlaying: false,
    isPlayingBuffer: false,
    
    init: function() {
        if (!this.audioContext) {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            this.gainNode = this.audioContext.createGain();
            this.gainNode.connect(this.audioContext.destination);
            this.gainNode.gain.value = 0.3;
        }
    },
    
    startTone: function(frequency, volume) {
        this.init();
        
        if (this.audioContext.state === 'suspended') {
            this.audioContext.resume();
        }
        
        if (!this.isPlaying) {
            this.oscillatorNode = this.audioContext.createOscillator();
            this.oscillatorNode.type = 'sine';
            this.oscillatorNode.frequency.value = frequency;
            this.oscillatorNode.connect(this.gainNode);
            this.oscillatorNode.start();
            this.isPlaying = true;
        } else {
            this.oscillatorNode.frequency.value = frequency;
        }
        
        this.gainNode.gain.value = volume;
    },
    
    stopTone: function() {
        if (this.isPlaying && this.oscillatorNode) {
            this.oscillatorNode.stop();
            this.oscillatorNode.disconnect();
            this.oscillatorNode = null;
            this.isPlaying = false;
        }
    },
    
    updateTone: function(frequency, volume) {
        if (this.isPlaying && this.oscillatorNode) {
            this.oscillatorNode.frequency.value = frequency;
            this.gainNode.gain.value = volume;
        }
    },
    
    // Play custom audio buffer from C++ memory
    playBuffer: function(bufferPtr, length, sampleRate, volume) {
        this.init();
        
        if (this.audioContext.state === 'suspended') {
            this.audioContext.resume();
        }
        
        // Stop any currently playing buffer first
        this.stopBuffer();
        
        // Create audio buffer
        var audioBuffer = this.audioContext.createBuffer(1, length, sampleRate);
        var channelData = audioBuffer.getChannelData(0);
        
        // Copy data from C++ memory (HEAPF32) to audio buffer
        for (var i = 0; i < length; i++) {
            channelData[i] = HEAPF32[(bufferPtr >> 2) + i];
        }
        
        // Create buffer source and connect
        this.bufferSourceNode = this.audioContext.createBufferSource();
        this.bufferSourceNode.buffer = audioBuffer;
        this.bufferSourceNode.connect(this.gainNode);
        this.bufferSourceNode.loop = true;
        
        this.gainNode.gain.value = volume;
        this.bufferSourceNode.start();
        this.isPlayingBuffer = true;
        
        var self = this;
        this.bufferSourceNode.onended = function() {
            self.isPlayingBuffer = false;
            self.bufferSourceNode = null;
        };
    },
    
    stopBuffer: function() {
        if (this.bufferSourceNode) {
            try {
                this.bufferSourceNode.stop();
                this.bufferSourceNode.disconnect();
            } catch (e) {
                // Already stopped or disconnected, ignore
            }
            this.bufferSourceNode = null;
        }
        this.isPlayingBuffer = false;
    },
    
    updateBufferVolume: function(volume) {
        if (this.gainNode) {
            this.gainNode.gain.value = volume;
        }
    }
};
