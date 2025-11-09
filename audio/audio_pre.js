// Web Audio API for tone generation
var AudioSynth = {
    audioContext: null,
    oscillatorNode: null,
    gainNode: null,
    isPlaying: false,
    
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
    }
};
