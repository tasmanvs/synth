#pragma once

#include "audio/nodes/audio_node.h"
#include <vector>

namespace audio_nodes {

// Keyboard Frequency node: Generates frequency values based on keyboard input
// Keys 1-9,0 select octave (sticky)
// Keys q-] select note within octave (plays while held)
// Supports configurable subdivisions (2-12) to limit active keys
class KeyboardFrequencyNode : public AudioNode {
public:
    KeyboardFrequencyNode(int node_id);
    
    std::vector<float> GenerateAudio(int num_samples, int sample_rate) override;
    void Draw() override;
    
private:
    float base_frequency_;
    int current_octave_;        // 0-9 for keys 1-0
    int current_note_;          // -1 for no note, 0-11 for keys a-l
    int num_subdivisions_;      // 2-12, number of active note keys
    
    float GetCurrentFrequency() const;
    float GetOctaveMultiplier() const;
    float GetNoteMultiplier() const;
};

} // namespace audio_nodes
