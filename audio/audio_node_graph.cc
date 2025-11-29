#include "audio/audio_node_graph.h"
#include "absl/log/log.h"
#include "imgui.h"

namespace audio_nodes {

// ============================================================================
// AudioNodeGraph Implementation
// ============================================================================

AudioNodeGraph::AudioNodeGraph(AudioInterface* audio_interface)
    : audio_interface_(audio_interface)
    , player_node_(nullptr)
    , editor_context_(nullptr)
    , next_node_id_(1)
    , next_link_id_(10000)
    , editor_initialized_(false) {
    
    // Initialize node editor
    ax::NodeEditor::Config config;
    editor_context_ = ax::NodeEditor::CreateEditor(&config);
    
    LOG(INFO) << "AudioNodeGraph created";
}

AudioNodeGraph::~AudioNodeGraph() {
    if (editor_context_) {
        ax::NodeEditor::DestroyEditor(editor_context_);
        editor_context_ = nullptr;
    }
}

SourceNode* AudioNodeGraph::CreateSourceNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<SourceNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 100 + 2, node_id); // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created source node: " << node_id;
    return node_ptr;
}

HarmonicNode* AudioNodeGraph::CreateHarmonicNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<HarmonicNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created harmonic node: " << node_id;
    return node_ptr;
}

SumNode* AudioNodeGraph::CreateSumNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<SumNode>(node_id, this);
    auto* node_ptr = node.get();
    
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 100 + 2, node_id);  // Input A
    RegisterPin(node_id * 100 + 3, node_id);  // Input B
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created sum node: " << node_id;
    return node_ptr;
}

BandpassFilterNode* AudioNodeGraph::CreateBandpassFilterNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<BandpassFilterNode>(node_id);
    auto* node_ptr = node.get();
    
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 500, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created bandpass filter node: " << node_id;
    return node_ptr;
}

LowpassFilterNode* AudioNodeGraph::CreateLowpassFilterNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<LowpassFilterNode>(node_id);
    auto* node_ptr = node.get();
    
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 501, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created lowpass filter node: " << node_id;
    return node_ptr;
}

HighpassFilterNode* AudioNodeGraph::CreateHighpassFilterNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<HighpassFilterNode>(node_id);
    auto* node_ptr = node.get();
    
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 502, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created highpass filter node: " << node_id;
    return node_ptr;
}

WhiteNoiseNode* AudioNodeGraph::CreateWhiteNoiseNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<WhiteNoiseNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created white noise node: " << node_id;
    return node_ptr;
}

NormalizerNode* AudioNodeGraph::CreateNormalizerNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<NormalizerNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 500, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created normalizer node: " << node_id;
    return node_ptr;
}

AmplitudeModulatorNode* AudioNodeGraph::CreateAmplitudeModulatorNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<AmplitudeModulatorNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 500, node_id);  // Carrier input pin
    RegisterPin(node_id * 1000 + 501, node_id);  // Modulator input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created amplitude modulator node: " << node_id;
    return node_ptr;
}

ScalerNode* AudioNodeGraph::CreateScalerNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<ScalerNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 500, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created scaler node: " << node_id;
    return node_ptr;
}

ReverbNode* AudioNodeGraph::CreateReverbNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<ReverbNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 500, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created reverb node: " << node_id;
    return node_ptr;
}

PitchShifterNode* AudioNodeGraph::CreatePitchShifterNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<PitchShifterNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 1000 + 500, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created pitch shifter node: " << node_id;
    return node_ptr;
}

FrequencySourceNode* AudioNodeGraph::CreateFrequencySourceNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<FrequencySourceNode>(node_id);
    auto* node_ptr = node.get();
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created frequency source node: " << node_id;
    return node_ptr;
}

PlayerNode* AudioNodeGraph::CreatePlayerNode() {
    // Only allow one player node
    if (player_node_) {
        LOG(WARNING) << "Player node already exists";
        return player_node_;
    }
    
    int node_id = next_node_id_++;
    auto node = std::make_unique<PlayerNode>(node_id, audio_interface_);
    player_node_ = node.get();
    
    RegisterPin(node_id * 100 + 2, node_id);  // Input pin
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created player node: " << node_id;
    return player_node_;
}

void AudioNodeGraph::DeleteNode(int node_id) {
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) return;
    
    // First, disconnect all input/output connections
    auto link_it = links_.begin();
    while (link_it != links_.end()) {
        int start_node_id = GetNodeIdForPin(link_it->start_pin_id);
        int end_node_id = GetNodeIdForPin(link_it->end_pin_id);
        
        if (start_node_id == node_id || end_node_id == node_id) {
            // Properly disconnect the nodes
            AudioNode* start_node = GetNode(start_node_id);
            AudioNode* end_node = GetNode(end_node_id);
            
            if (start_node && end_node) {
                end_node->RemoveInput(start_node, link_it->end_pin_id);
            }
            
            link_it = links_.erase(link_it);
        } else {
            ++link_it;
        }
    }
    
    // Clear player node reference if this is the player
    if (player_node_ && player_node_->GetNodeId() == node_id) {
        player_node_ = nullptr;
    }
    
    UnregisterPinsForNode(node_id);
    nodes_.erase(it);
    
    LOG(INFO) << "Deleted node: " << node_id;
}

AudioNode* AudioNodeGraph::GetNode(int node_id) {
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool AudioNodeGraph::CreateLink(int start_pin_id, int end_pin_id) {
    // Get the nodes for these pins
    AudioNode* start_node = GetNodeForPin(start_pin_id);
    AudioNode* end_node = GetNodeForPin(end_pin_id);
    
    if (!start_node || !end_node) {
        LOG(WARNING) << "Could not find nodes for pins " << start_pin_id << " -> " << end_pin_id;
        return false;
    }
    
    // Verify start pin is an output and end pin is an input
    if (!IsPinOutput(start_pin_id) || IsPinOutput(end_pin_id)) {
        LOG(WARNING) << "Invalid pin types for link";
        return false;
    }
    
    // Add the input connection
    if (!end_node->AddInput(start_node, end_pin_id)) {
        LOG(WARNING) << "Failed to add input to node";
        return false;
    }
    
    // Create the link
    Link link;
    link.link_id = next_link_id_++;
    link.start_pin_id = start_pin_id;
    link.end_pin_id = end_pin_id;
    links_.push_back(link);
    
    LOG(INFO) << "Created link: " << link.link_id << " from pin " << start_pin_id << " to " << end_pin_id;
    return true;
}

void AudioNodeGraph::DeleteLink(int link_id) {
    auto it = std::find_if(links_.begin(), links_.end(),
        [link_id](const Link& link) { return link.link_id == link_id; });
    
    if (it == links_.end()) return;
    
    // Remove the input connection
    AudioNode* start_node = GetNodeForPin(it->start_pin_id);
    AudioNode* end_node = GetNodeForPin(it->end_pin_id);
    
    if (start_node && end_node) {
        end_node->RemoveInput(start_node, it->end_pin_id);
    }
    
    links_.erase(it);
    
    LOG(INFO) << "Deleted link: " << link_id;
}

int AudioNodeGraph::GetNodeIdForPin(int pin_id) {
    auto it = pin_to_node_map_.find(pin_id);
    if (it != pin_to_node_map_.end()) {
        return it->second;
    }
    return -1;
}

AudioNode* AudioNodeGraph::GetNodeForPin(int pin_id) {
    int node_id = GetNodeIdForPin(pin_id);
    return GetNode(node_id);
}

bool AudioNodeGraph::IsPinOutput(int pin_id) {
    int node_id = GetNodeIdForPin(pin_id);
    if (node_id < 0) return false;
    
    AudioNode* node = GetNode(node_id);
    if (!node) return false;
    
    // Output pin is always node_id * 100 + 1
    return pin_id == node->GetOutputPinId();
}

void AudioNodeGraph::Update(int sample_rate) {
    if (!player_node_) {
        return;
    }

    // Don't call OnGraphChanged() for parameter changes - the phase-continuous
    // generators handle smooth transitions without needing to clear buffers
    // HasGraphChanged() is still checked to reset the change flags
    HasGraphChanged();

    player_node_->UpdateAudio(sample_rate);
}

bool AudioNodeGraph::HasGraphChanged() {
    for (auto& [node_id, node] : nodes_) {
        if (node->GetNodeType() == NodeType::kSource) {
            SourceNode* source = static_cast<SourceNode*>(node.get());
            if (source->HasParametersChanged()) {
                source->ResetChangeFlag();
                return true;
            }
        } else if (node->GetNodeType() == NodeType::kHarmonic) {
            HarmonicNode* harmonic = static_cast<HarmonicNode*>(node.get());
            if (harmonic->HasParametersChanged()) {
                harmonic->ResetChangeFlag();
                return true;
            }
        }
    }
    return false;
}

void AudioNodeGraph::Draw() {
    namespace ed = ax::NodeEditor;
    
    ed::SetCurrentEditor(editor_context_);
    ed::Begin("Audio Node Editor", ImVec2(0.0f, 0.0f));
    
    // Draw all nodes
    for (auto& [node_id, node] : nodes_) {
        node->Draw();
    }
    
    // Set initial positions only once
    if (!editor_initialized_ && !nodes_.empty()) {
        float x_pos = 100.0f;
        float y_pos = 100.0f;
        
        for (auto& [node_id, node] : nodes_) {
            ed::SetNodePosition(node_id, ImVec2(x_pos, y_pos));
            x_pos += 250.0f;
            if (x_pos > 700.0f) {
                x_pos = 100.0f;
                y_pos += 200.0f;
            }
        }
        
        ed::NavigateToContent(0.0f);
        editor_initialized_ = true;
    }
    
    // Draw all links
    for (const auto& link : links_) {
        ed::Link(link.link_id, link.start_pin_id, link.end_pin_id);
    }
    
    // Handle link creation
    if (ed::BeginCreate()) {
        ed::PinId start_pin_id, end_pin_id;
        if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) {
            if (start_pin_id && end_pin_id) {
                if (ed::AcceptNewItem()) {
                    CreateLink(start_pin_id.Get(), end_pin_id.Get());
                }
            }
        }
    }
    ed::EndCreate();
    
    // Handle link deletion
    if (ed::BeginDelete()) {
        ed::LinkId deleted_link_id;
        while (ed::QueryDeletedLink(&deleted_link_id)) {
            if (ed::AcceptDeletedItem()) {
                DeleteLink(deleted_link_id.Get());
            }
        }
        
        // Handle node deletion
        ed::NodeId deleted_node_id;
        while (ed::QueryDeletedNode(&deleted_node_id)) {
            if (ed::AcceptDeletedItem()) {
                DeleteNode(deleted_node_id.Get());
            }
        }
    }
    ed::EndDelete();
    
    // Handle context menu for creating new nodes
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("Create Node");
    }
    if (ImGui::BeginPopup("Create Node")) {
        if (ImGui::MenuItem("Source Node")) {
            CreateSourceNode();
        }
        if (ImGui::MenuItem("Harmonic Node")) {
            CreateHarmonicNode();
        }
        if (ImGui::MenuItem("White Noise")) {
            CreateWhiteNoiseNode();
        }
        if (ImGui::MenuItem("Normalizer")) {
            CreateNormalizerNode();
        }
        if (ImGui::MenuItem("Amplitude Modulator")) {
            CreateAmplitudeModulatorNode();
        }
        if (ImGui::MenuItem("Scaler")) {
            CreateScalerNode();
        }
        if (ImGui::MenuItem("Reverb")) {
            CreateReverbNode();
        }
        if (ImGui::MenuItem("Pitch Shifter")) {
            CreatePitchShifterNode();
        }
        if (ImGui::MenuItem("Frequency Source")) {
            CreateFrequencySourceNode();
        }
        if (ImGui::MenuItem("Sum Node")) {
            CreateSumNode();
        }
        if (ImGui::MenuItem("Lowpass Filter")) {
            CreateLowpassFilterNode();
        }
        if (ImGui::MenuItem("Highpass Filter")) {
            CreateHighpassFilterNode();
        }
        if (ImGui::MenuItem("Bandpass Filter")) {
            CreateBandpassFilterNode();
        }
        if (ImGui::MenuItem("Player Node")) {
            if (!player_node_) {
                CreatePlayerNode();
            } else {
                ImGui::Text("Player node already exists!");
            }
        }
        ImGui::EndPopup();
    }
    ed::Resume();
    
    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void AudioNodeGraph::RegisterPin(int pin_id, int node_id) {
    pin_to_node_map_[pin_id] = node_id;
}

void AudioNodeGraph::UnregisterPinsForNode(int node_id) {
    auto it = pin_to_node_map_.begin();
    while (it != pin_to_node_map_.end()) {
        if (it->second == node_id) {
            it = pin_to_node_map_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace audio_nodes
