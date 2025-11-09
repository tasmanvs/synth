// Web Audio API integration for audio synthesis
var LibraryAudio = {
    js_init_audio: function() {
        AudioSynth.init();
    },
    
    js_start_tone: function(frequency, volume) {
        AudioSynth.startTone(frequency, volume);
    },
    
    js_stop_tone: function() {
        AudioSynth.stopTone();
    },
    
    js_update_tone: function(frequency, volume) {
        AudioSynth.updateTone(frequency, volume);
    }
};

mergeInto(LibraryManager.library, LibraryAudio);
