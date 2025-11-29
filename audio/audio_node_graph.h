#pragma once

#include "imgui_node_editor.h"
#include "audio/audio_interface.h"
#include "audio/nodes/audio_node.h"
#include "audio/nodes/source_node.h"
#include "audio/nodes/frequency_source_node.h"
#include "audio/nodes/harmonic_node.h"
#include "audio/nodes/filter_nodes.h"
#include "audio/nodes/effect_nodes.h"
#include "audio/nodes/sum_node.h"
#include "audio/nodes/player_node.h"

#include <vector>
#include <memory>
#include <map>

namespace audio_nodes {

// Audio node graph manager
class AudioNodeGraph {
public:
    AudioNodeGraph(AudioInterface* audio_interface);
    ~AudioNodeGraph();
    
    // Node creation
    SourceNode* CreateSourceNode();
    HarmonicNode* CreateHarmonicNode();
    SumNode* CreateSumNode();
    PlayerNode* CreatePlayerNode();
    BandpassFilterNode* CreateBandpassFilterNode();
    LowpassFilterNode* CreateLowpassFilterNode();
    HighpassFilterNode* CreateHighpassFilterNode();
    WhiteNoiseNode* CreateWhiteNoiseNode();
    NormalizerNode* CreateNormalizerNode();
    AmplitudeModulatorNode* CreateAmplitudeModulatorNode();
    ScalerNode* CreateScalerNode();
    ReverbNode* CreateReverbNode();
    PitchShifterNode* CreatePitchShifterNode();
    FrequencySourceNode* CreateFrequencySourceNode();
    
    // Node management
    void DeleteNode(int node_id);
    AudioNode* GetNode(int node_id);
    
    // Link management
    bool CreateLink(int start_pin_id, int end_pin_id);
    void DeleteLink(int link_id);
    
    // Get pin information
    int GetNodeIdForPin(int pin_id);
    AudioNode* GetNodeForPin(int pin_id);
    bool IsPinOutput(int pin_id);
    
    // Update and draw
    void Update(int sample_rate);
    void Draw();
    
    // Check if any parameters have changed
    bool HasGraphChanged();
    
    // Access the editor context
    ax::NodeEditor::EditorContext* GetEditorContext() { return editor_context_; }
    
    // Access nodes (for external UI)
    const std::map<int, std::unique_ptr<AudioNode>>& GetNodes() const { return nodes_; }
    
    // Register a pin to a node (used by nodes when creating dynamic pins)
    void RegisterPin(int pin_id, int node_id);

private:
    struct Link {
        int link_id;
        int start_pin_id;
        int end_pin_id;
    };
    
    std::map<int, std::unique_ptr<AudioNode>> nodes_;
    std::vector<Link> links_;
    std::map<int, int> pin_to_node_map_;
    
    AudioInterface* audio_interface_;
    PlayerNode* player_node_;
    ax::NodeEditor::EditorContext* editor_context_;
    
    int next_node_id_;
    int next_link_id_;
    
    bool editor_initialized_;
    
    void UnregisterPinsForNode(int node_id);

    friend class SumNode;
};

} // namespace audio_nodes
