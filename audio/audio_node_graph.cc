#include "audio/audio_node_graph.h"
#include "absl/log/log.h"
#include <algorithm>

namespace audio_nodes {

// ============================================================================
// AudioNode Base Class
// ============================================================================

AudioNode::AudioNode(int node_id, NodeType type)
    : node_id_(node_id)
    , output_pin_id_(node_id * 100 + 1)  // Simple pin ID scheme
    , type_(type) {
}

// ============================================================================
// SourceNode Implementation
// ============================================================================

SourceNode::SourceNode(int node_id)
    : AudioNode(node_id, NodeType::kSource)
    , frequency_(440.0f)
    , volume_(0.5f)
    , last_frequency_(440.0f)
    , last_volume_(0.5f)
    , parameters_changed_(false)
    , phase_generator_(0.0f) {
    buffer_config_.frequency_hz = frequency_;
    buffer_config_.amplitude = volume_;
    buffer_config_.sample_rate = 48000;
    buffer_config_.frame_count = 512;
}

std::vector<float> SourceNode::GenerateAudio(int num_samples, int sample_rate) {
    if (num_samples <= 0 || sample_rate <= 0) {
        return {};
    }

    buffer_config_.frame_count = num_samples;
    buffer_config_.sample_rate = sample_rate;
    buffer_config_.frequency_hz = frequency_;
    buffer_config_.amplitude = volume_;

    return phase_generator_.GenerateBuffer(buffer_config_);
}

void SourceNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Source Node %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    if (ImGui::SliderFloat("Frequency", &frequency_, 20.0f, 2000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f")) {
        parameters_changed_ = true;
    }
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

bool SourceNode::HasParametersChanged() {
    if (frequency_ != last_frequency_ || volume_ != last_volume_) {
        last_frequency_ = frequency_;
        last_volume_ = volume_;
        return true;
    }
    return parameters_changed_;
}

// ============================================================================
// SumNode Implementation
// ============================================================================

SumNode::SumNode(int node_id)
    : AudioNode(node_id, NodeType::kSum)
    , input_pin_a_id_(node_id * 100 + 2)
    , input_pin_b_id_(node_id * 100 + 3)
    , input_a_(nullptr)
    , input_b_(nullptr) {
}

std::vector<float> SumNode::GenerateAudio(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    
    if (input_a_) {
        auto input_a_data = input_a_->GenerateAudio(num_samples, sample_rate);
        for (int i = 0; i < num_samples; i++) {
            output[i] += input_a_data[i];
        }
    }
    
    if (input_b_) {
        auto input_b_data = input_b_->GenerateAudio(num_samples, sample_rate);
        for (int i = 0; i < num_samples; i++) {
            output[i] += input_b_data[i];
        }
    }
    
    // Clamp to prevent overflow
    for (int i = 0; i < num_samples; i++) {
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
    }
    
    return output;
}

void SumNode::Draw() {
    namespace ed = ax::NodeEditor;
    
    ImGui::PushID(node_id_);
    ed::BeginNode(node_id_);
    
    ImGui::Text("Sum Node %d", node_id_);
    
    // Input pins
    ed::BeginPin(input_pin_a_id_, ed::PinKind::Input);
    ImGui::Text("-> A");
    ed::EndPin();
    
    ed::BeginPin(input_pin_b_id_, ed::PinKind::Input);
    ImGui::Text("-> B");
    ed::EndPin();
    
    ImGui::Text("  +  ");
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

bool SumNode::AddInput(AudioNode* input_node) {
    if (!input_node) return false;
    
    if (!input_a_) {
        input_a_ = input_node;
        return true;
    } else if (!input_b_) {
        input_b_ = input_node;
        return true;
    }
    
    return false;
}

void SumNode::RemoveInput(AudioNode* input_node) {
    if (input_a_ == input_node) {
        input_a_ = nullptr;
    }
    if (input_b_ == input_node) {
        input_b_ = nullptr;
    }
}

// ============================================================================
// PlayerNode Implementation
// ============================================================================

PlayerNode::PlayerNode(int node_id, AudioInterface* audio_interface)
    : AudioNode(node_id, NodeType::kPlayer)
    , input_pin_id_(node_id * 100 + 2)
    , input_(nullptr)
    , audio_interface_(audio_interface)
    , playing_(false)
    , volume_(0.5f)
    , playback_history_()
    , history_limit_samples_(48000 * 5)
    , show_debug_window_(false)
    , plot_scratch_buffer_()
    , max_plot_samples_(16000) {
}

std::vector<float> PlayerNode::GenerateAudio(int num_samples, int sample_rate) {
    if (input_) {
        return input_->GenerateAudio(num_samples, sample_rate);
    }
    return std::vector<float>(num_samples, 0.0f);
}

void PlayerNode::Draw() {
    namespace ed = ax::NodeEditor;
    
    ImGui::PushID(node_id_);
    ed::BeginNode(node_id_);
    
    ImGui::Text("Player Node %d", node_id_);
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    
    ImGui::Separator();
    
    if (ImGui::Button(playing_ ? "Stop" : "Play")) {
        SetPlaying(!playing_);
    }
    
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat("Master", &volume_, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();

    if (ImGui::Button("Show Buffer Debug")) {
        show_debug_window_ = true;
    }
    
    ed::EndNode();
    ImGui::PopID();

    if (show_debug_window_) {
        ed::Suspend();
        DrawHistoryWindow();
        ed::Resume();
    }
}

bool PlayerNode::AddInput(AudioNode* input_node) {
    if (!input_node) return false;
    
    if (!input_) {
        input_ = input_node;
        return true;
    }
    
    return false;
}

void PlayerNode::RemoveInput(AudioNode* input_node) {
    if (input_ == input_node) {
        input_ = nullptr;
    }
}

void PlayerNode::SetPlaying(bool playing) {
    playing_ = playing;
    
    if (!playing_) {
        audio_interface_->stop();
        LOG(INFO) << "Player node stopped";
    } else {
        LOG(INFO) << "Player node started";
    }
}

void PlayerNode::UpdateAudio(int sample_rate) {
    if (!playing_ || !audio_interface_) return;
    
    // Generate a buffer of audio with current parameters
    const int buffer_size = sample_rate / 10;  // 100ms buffer
    auto audio_data = GenerateAudio(buffer_size, sample_rate);
    
    // Apply master volume
    for (auto& sample : audio_data) {
        sample *= volume_;
    }

    AppendToHistory(audio_data);
    
    // Convert float to short (16-bit PCM)
    std::vector<short> short_data(buffer_size);
    for (int i = 0; i < buffer_size; i++) {
        float clamped = std::max(-1.0f, std::min(1.0f, audio_data[i]));
        short_data[i] = static_cast<short>(clamped * 32767.0f);
    }
    
    // Stop current audio, update buffer, and restart
    // This is necessary because OpenAL doesn't allow buffer updates while playing
    audio_interface_->stop();
    audio_interface_->playSamples(short_data, sample_rate, true);
    audio_interface_->play();
}

void PlayerNode::AppendToHistory(const std::vector<float>& samples) {
    if (samples.empty()) {
        return;
    }

    playback_history_.insert(playback_history_.end(), samples.begin(), samples.end());

    if (playback_history_.size() > history_limit_samples_) {
        size_t excess = playback_history_.size() - history_limit_samples_;
        playback_history_.erase(playback_history_.begin(), playback_history_.begin() + excess);
    }
}

void PlayerNode::DrawHistoryWindow() {
    if (!show_debug_window_) {
        return;
    }

    if (!ImGui::Begin("Player Buffer Debug", &show_debug_window_)) {
        ImGui::End();
        return;
    }

    if (playback_history_.empty()) {
        ImGui::Text("No buffers captured yet.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear History")) {
        playback_history_.clear();
    }
    ImGui::SameLine();
    ImGui::Text("Captured samples: %zu", playback_history_.size());

    int sample_count = 0;
    const float* plot_data = PreparePlotData(&sample_count);
    if (!plot_data || sample_count <= 0) {
        ImGui::Text("No data available for plotting.");
        ImGui::End();
        return;
    }

    ImGui::Text("Plotting %d points (downsampled from %zu)", sample_count,
                playback_history_.size());

    if (ImPlot::BeginPlot("Captured Buffers", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Sample", "Amplitude",
                          ImPlotAxisFlags_NoGridLines,
                          ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxesLimits(0.0,
                                static_cast<double>(sample_count),
                                -1.1,
                                1.1,
                                ImPlotCond_Always);
        ImPlot::PlotLine("History",
                         plot_data,
                         sample_count);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

const float* PlayerNode::PreparePlotData(int* sample_count) {
    if (sample_count == nullptr) {
        return nullptr;
    }

    if (playback_history_.empty()) {
        *sample_count = 0;
        return nullptr;
    }

    if (playback_history_.size() <= max_plot_samples_) {
        *sample_count = static_cast<int>(playback_history_.size());
        return playback_history_.data();
    }

    const size_t stride =
        (playback_history_.size() + max_plot_samples_ - 1) / max_plot_samples_;
    const size_t downsampled_count =
        (playback_history_.size() + stride - 1) / stride;
    plot_scratch_buffer_.resize(downsampled_count);

    size_t idx = 0;
    for (size_t i = 0; i < playback_history_.size(); i += stride) {
        plot_scratch_buffer_[idx++] = playback_history_[i];
    }

    *sample_count = static_cast<int>(idx);
    return plot_scratch_buffer_.data();
}

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
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created source node: " << node_id;
    return node_ptr;
}

SumNode* AudioNodeGraph::CreateSumNode() {
    int node_id = next_node_id_++;
    auto node = std::make_unique<SumNode>(node_id);
    auto* node_ptr = node.get();
    
    RegisterPin(node_ptr->GetOutputPinId(), node_id);
    RegisterPin(node_id * 100 + 2, node_id);  // Input A
    RegisterPin(node_id * 100 + 3, node_id);  // Input B
    
    nodes_[node_id] = std::move(node);
    
    LOG(INFO) << "Created sum node: " << node_id;
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
    
    // Remove all links connected to this node
    auto link_it = links_.begin();
    while (link_it != links_.end()) {
        int start_node = GetNodeIdForPin(link_it->start_pin_id);
        int end_node = GetNodeIdForPin(link_it->end_pin_id);
        
        if (start_node == node_id || end_node == node_id) {
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
    if (!end_node->AddInput(start_node)) {
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
        end_node->RemoveInput(start_node);
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
    // Update player node audio if it exists and is playing
    if (player_node_ && player_node_->IsPlaying()) {
        // Check if any parameters have changed
        if (HasGraphChanged()) {
            // Force audio update
            player_node_->UpdateAudio(sample_rate);
        }
    }
}

bool AudioNodeGraph::HasGraphChanged() {
    for (auto& [node_id, node] : nodes_) {
        if (node->GetNodeType() == NodeType::kSource) {
            SourceNode* source = static_cast<SourceNode*>(node.get());
            if (source->HasParametersChanged()) {
                source->ResetChangeFlag();
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
        if (ImGui::MenuItem("Sum Node")) {
            CreateSumNode();
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
