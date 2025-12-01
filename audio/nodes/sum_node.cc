#include "audio/nodes/sum_node.h"
#include "audio/audio_node_graph.h"
#include "audio/nodes/source_node.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include "absl/log/log.h"
#include <algorithm>

namespace audio_nodes {

SumNode::SumNode(int node_id, AudioNodeGraph* graph)
    : AudioNode(node_id, NodeType::kSum)
    , graph_(graph)
    , next_pin_offset_(0) {
    AddInputSlot();
    AddInputSlot();
}

std::vector<float> SumNode::GenerateAudio(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    
    for (const auto& slot : inputs_) {
        if (!slot.input) {
            continue;
        }
        auto input_data = slot.input->GenerateAudio(num_samples, sample_rate);
        for (int i = 0; i < num_samples && i < static_cast<int>(input_data.size()); ++i) {
            output[i] += input_data[i];
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
    int slot_index = 1;
    InputSlot* first_free_slot = nullptr;
    for (auto& slot : inputs_) {
        if (!slot.input && !first_free_slot) {
            first_free_slot = &slot;
        }
        ed::BeginPin(slot.pin_id, ed::PinKind::Input);
        ImGui::Text("-> In %d", slot_index++);
        ed::EndPin();
    }

    if (ImGui::Button("Add Input Slot")) {
        AddInputSlot();
    }

    bool can_spawn = graph_ != nullptr;
    if (!first_free_slot && graph_) {
        if (AddInputSlot()) {
            first_free_slot = &inputs_.back();
        } else {
            can_spawn = false;
        }
    }

    ImGui::SameLine();
    if (!can_spawn || !first_free_slot) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Spawn Source")) {
        if (graph_) {
            SourceNode* new_source = graph_->CreateSourceNode();
            if (new_source && first_free_slot) {
                int start_pin = new_source->GetOutputPinId();
                if (!graph_->CreateLink(start_pin, first_free_slot->pin_id)) {
                    LOG(WARNING) << "Failed to link new source to sum node " << node_id_;
                }
            }
        }
    }
    if (!can_spawn || !first_free_slot) {
        ImGui::EndDisabled();
    }

    ImGui::Text("  +  ");
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

bool SumNode::AddInput(AudioNode* input_node, int pin_id) {
    if (!input_node) {
        return false;
    }

    InputSlot* target = nullptr;
    if (pin_id >= 0) {
        target = FindSlotByPin(pin_id);
        if (target && target->input != nullptr) {
            target = nullptr;
        }
    }
    if (!target) {
        for (auto& slot : inputs_) {
            if (slot.input == nullptr) {
                target = &slot;
                break;
            }
        }
    }

    if (!target) {
        if (!AddInputSlot()) {
            return false;
        }
        target = &inputs_.back();
    }

    if (target->input != nullptr) {
        return false;
    }

    target->input = input_node;
    return true;
}

void SumNode::RemoveInput(AudioNode* input_node, int pin_id) {
    InputSlot* target = nullptr;
    if (pin_id >= 0) {
        target = FindSlotByPin(pin_id);
    }

    if (!target) {
        for (auto& slot : inputs_) {
            if (slot.input == input_node) {
                target = &slot;
                break;
            }
        }
    }

    if (target) {
        target->input = nullptr;
    }
}

SumNode::InputSlot* SumNode::FindSlotByPin(int pin_id) {
    for (auto& slot : inputs_) {
        if (slot.pin_id == pin_id) {
            return &slot;
        }
    }
    return nullptr;
}

bool SumNode::AddInputSlot() {
    int pin_id = node_id_ * 100 + 2 + next_pin_offset_;
    next_pin_offset_++;
    inputs_.push_back({pin_id, nullptr});
    
    if (graph_) {
        graph_->RegisterPin(pin_id, node_id_);
    }
    return true;
}

} // namespace audio_nodes
