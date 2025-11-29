#include "audio/nodes/effect_nodes.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace audio_nodes {

// ============================================================================
// WhiteNoiseNode Implementation
// ============================================================================

WhiteNoiseNode::WhiteNoiseNode(int node_id)
    : AudioNode(node_id, NodeType::kWhiteNoise)
    , volume_(0.3f) {
}

std::vector<float> WhiteNoiseNode::GenerateAudio(int num_samples, int sample_rate) {
    if (num_samples <= 0) {
        return {};
    }
    
    std::vector<float> output(num_samples);
    
    // Generate white noise: random values between -1 and 1
    for (int i = 0; i < num_samples; ++i) {
        // Generate random float between -1.0 and 1.0
        float random_value = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        random_value = (random_value * 2.0f - 1.0f) * volume_;
        output[i] = random_value;
    }
    
    return output;
}

void WhiteNoiseNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("White Noise %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

// ============================================================================
// NormalizerNode Implementation
// ============================================================================

NormalizerNode::NormalizerNode(int node_id)
    : AudioNode(node_id, NodeType::kNormalizer)
    , input_pin_id_(node_id * 1000 + 500)
    , input_(nullptr)
    , target_level_(0.8f)
    , attack_time_(0.01f)
    , release_time_(0.1f)
    , current_gain_(1.0f)
    , peak_level_(0.0f)
    , smoothing_factor_(0.99f) {
}

bool NormalizerNode::AddInput(AudioNode* input_node, int pin_id) {
    if (input_ != nullptr) {
        return false;  // Already have an input
    }
    input_ = input_node;
    return true;
}

void NormalizerNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node) {
        input_ = nullptr;
    }
}

std::vector<float> NormalizerNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // Get input audio
    std::vector<float> input_audio = input_->GenerateAudio(num_samples, sample_rate);
    if (input_audio.empty()) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    std::vector<float> output(num_samples);
    
    // Calculate attack and release coefficients based on sample rate
    float attack_coeff = 1.0f - std::exp(-1.0f / (attack_time_ * sample_rate));
    float release_coeff = 1.0f - std::exp(-1.0f / (release_time_ * sample_rate));
    
    // Process each sample
    for (int i = 0; i < num_samples; ++i) {
        float input_sample = input_audio[i];
        float abs_sample = std::abs(input_sample);
        
        // Update peak level with attack/release
        if (abs_sample > peak_level_) {
            peak_level_ += attack_coeff * (abs_sample - peak_level_);
        } else {
            peak_level_ += release_coeff * (abs_sample - peak_level_);
        }
        
        // Calculate desired gain
        float desired_gain = 1.0f;
        if (peak_level_ > 0.0001f) {
            desired_gain = target_level_ / peak_level_;
            // Limit gain to reasonable range (0.1 to 1000.0)
            desired_gain = std::max(0.1f, std::min(1000.0f, desired_gain));
        }
        
        // Smooth the gain changes
        current_gain_ = smoothing_factor_ * current_gain_ + (1.0f - smoothing_factor_) * desired_gain;
        
        // Apply gain
        output[i] = input_sample * current_gain_;
    }
    
    return output;
}

void NormalizerNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Normalizer %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    
    ImGui::SliderFloat("Target Level", &target_level_, 0.1f, 1.0f, "%.2f");
    ImGui::SliderFloat("Attack (ms)", &attack_time_, 0.001f, 0.1f, "%.3f");
    ImGui::SliderFloat("Release (ms)", &release_time_, 0.01f, 1.0f, "%.3f");
    
    // Display current gain
    ImGui::Text("Current Gain: %.2fx", current_gain_);
    ImGui::Text("Peak Level: %.2f", peak_level_);
    
    ImGui::PopItemWidth();
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("<- Input");
    ed::EndPin();
    
    ImGui::SameLine();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Output ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

// ============================================================================
// AmplitudeModulatorNode Implementation
// ============================================================================

AmplitudeModulatorNode::AmplitudeModulatorNode(int node_id)
    : AudioNode(node_id, NodeType::kAmplitudeModulator)
    , carrier_pin_id_(node_id * 1000 + 500)
    , modulator_pin_id_(node_id * 1000 + 501)
    , carrier_input_(nullptr)
    , modulator_input_(nullptr)
    , modulation_depth_(1.0f)
    , dc_offset_(0.5f) {
}

bool AmplitudeModulatorNode::AddInput(AudioNode* input_node, int pin_id) {
    if (pin_id == carrier_pin_id_ && carrier_input_ == nullptr) {
        carrier_input_ = input_node;
        return true;
    } else if (pin_id == modulator_pin_id_ && modulator_input_ == nullptr) {
        modulator_input_ = input_node;
        return true;
    }
    return false;
}

void AmplitudeModulatorNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (carrier_input_ == input_node && pin_id == carrier_pin_id_) {
        carrier_input_ = nullptr;
    } else if (modulator_input_ == input_node && pin_id == modulator_pin_id_) {
        modulator_input_ = nullptr;
    }
}

std::vector<float> AmplitudeModulatorNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!carrier_input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // Get carrier signal
    std::vector<float> carrier = carrier_input_->GenerateAudio(num_samples, sample_rate);
    if (carrier.empty()) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // If no modulator, just pass through carrier
    if (!modulator_input_) {
        return carrier;
    }
    
    // Get modulator signal
    std::vector<float> modulator = modulator_input_->GenerateAudio(num_samples, sample_rate);
    if (modulator.empty()) {
        return carrier;
    }
    
    std::vector<float> output(num_samples);
    
    // Apply amplitude modulation: output = carrier * (dc_offset + modulation_depth * modulator)
    for (int i = 0; i < num_samples; ++i) {
        float mod_signal = modulator[i] * modulation_depth_ + dc_offset_;
        // Clamp modulation signal to prevent extreme values
        mod_signal = std::max(0.0f, std::min(2.0f, mod_signal));
        output[i] = carrier[i] * mod_signal;
    }
    
    return output;
}

void AmplitudeModulatorNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Amplitude Modulator %d", node_id_);
    ImGui::PushItemWidth(140.0f);
    
    ImGui::SliderFloat("Mod Depth", &modulation_depth_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("DC Offset", &dc_offset_, 0.0f, 1.0f, "%.2f");
    
    ImGui::PopItemWidth();
    
    // Carrier input pin
    ed::BeginPin(carrier_pin_id_, ed::PinKind::Input);
    ImGui::Text("<- Carrier");
    ed::EndPin();
    
    // Modulator input pin
    ed::BeginPin(modulator_pin_id_, ed::PinKind::Input);
    ImGui::Text("<- Modulator");
    ed::EndPin();
    
    ImGui::SameLine();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Output ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

// ============================================================================
// ScalerNode Implementation
// ============================================================================

ScalerNode::ScalerNode(int node_id)
    : AudioNode(node_id, NodeType::kScaler)
    , input_pin_id_(node_id * 1000 + 500)
    , input_(nullptr)
    , input_min_(-1.0f)
    , input_max_(1.0f)
    , output_min_(0.0f)
    , output_max_(1.0f)
    , auto_detect_range_(false)
    , detected_min_(0.0f)
    , detected_max_(0.0f) {
}

bool ScalerNode::AddInput(AudioNode* input_node, int pin_id) {
    if (input_ != nullptr) {
        return false;  // Already have an input
    }
    input_ = input_node;
    return true;
}

void ScalerNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node) {
        input_ = nullptr;
        // Reset detection when input is removed
        detected_min_ = 0.0f;
        detected_max_ = 0.0f;
    }
}

std::vector<float> ScalerNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // Get input audio
    std::vector<float> input_audio = input_->GenerateAudio(num_samples, sample_rate);
    if (input_audio.empty()) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    std::vector<float> output(num_samples);
    
    float actual_input_min = input_min_;
    float actual_input_max = input_max_;
    
    // Auto-detect input range if enabled
    if (auto_detect_range_) {
        // Initialize on first sample
        if (detected_min_ == 0.0f && detected_max_ == 0.0f) {
            detected_min_ = input_audio[0];
            detected_max_ = input_audio[0];
        }
        
        // Update detected range with decay towards current values
        for (int i = 0; i < num_samples; ++i) {
            float sample = input_audio[i];
            if (sample < detected_min_) {
                detected_min_ = sample;
            } else {
                detected_min_ = detected_min_ * 0.9999f + sample * 0.0001f;
            }
            
            if (sample > detected_max_) {
                detected_max_ = sample;
            } else {
                detected_max_ = detected_max_ * 0.9999f + sample * 0.0001f;
            }
        }
        
        actual_input_min = detected_min_;
        actual_input_max = detected_max_;
    }
    
    // Compute scaling parameters
    float input_range = actual_input_max - actual_input_min;
    float output_range = output_max_ - output_min_;
    
    // Avoid division by zero
    if (std::abs(input_range) < 0.0001f) {
        input_range = 1.0f;
    }
    
    // Apply linear remapping: output = output_min + (input - input_min) * (output_range / input_range)
    for (int i = 0; i < num_samples; ++i) {
        float normalized = (input_audio[i] - actual_input_min) / input_range;
        output[i] = output_min_ + normalized * output_range;
    }
    
    return output;
}

void ScalerNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Scaler %d", node_id_);
    ImGui::PushItemWidth(140.0f);
    
    ImGui::Text("Output Range:");
    ImGui::DragFloat("Min##out", &output_min_, 0.01f, -10.0f, 10.0f, "%.2f");
    ImGui::DragFloat("Max##out", &output_max_, 0.01f, -10.0f, 10.0f, "%.2f");
    
    ImGui::Separator();
    
    if (ImGui::Checkbox("Auto-Detect Input", &auto_detect_range_)) {
        if (auto_detect_range_) {
            // Reset detection when enabling
            detected_min_ = 0.0f;
            detected_max_ = 0.0f;
        }
    }
    
    if (!auto_detect_range_) {
        ImGui::Text("Input Range:");
        ImGui::DragFloat("Min##in", &input_min_, 0.01f, -10.0f, 10.0f, "%.2f");
        ImGui::DragFloat("Max##in", &input_max_, 0.01f, -10.0f, 10.0f, "%.2f");
    } else {
        ImGui::Text("Detected: [%.2f, %.2f]", detected_min_, detected_max_);
    }
    
    ImGui::PopItemWidth();
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("<- Input");
    ed::EndPin();
    
    ImGui::SameLine();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Output ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

// ============================================================================
// ReverbNode Implementation
// ============================================================================

ReverbNode::ReverbNode(int node_id)
    : AudioNode(node_id, NodeType::kReverb)
    , input_pin_id_(node_id * 1000 + 500)
    , input_(nullptr)
    , room_size_(0.5f)
    , damping_(0.5f)
    , wet_level_(0.3f)
    , dry_level_(0.7f)
    , buffers_initialized_(false)
    , last_sample_rate_(0) {
    comb_buffers_.resize(kNumCombs_);
    comb_indices_.resize(kNumCombs_, 0);
    comb_feedback_.resize(kNumCombs_, 0.0f);
    comb_damp_.resize(kNumCombs_, 0.0f);
    
    allpass_buffers_.resize(kNumAllpass_);
    allpass_indices_.resize(kNumAllpass_, 0);
}

void ReverbNode::InitializeBuffers(int sample_rate) {
    if (buffers_initialized_ && last_sample_rate_ == sample_rate) {
        return;
    }
    
    // Comb filter delays (in samples) - tuned for pleasant reverb
    // Based on Freeverb parameters scaled to sample rate
    const int comb_delays[kNumCombs_] = {
        static_cast<int>(1557 * sample_rate / 44100.0f),
        static_cast<int>(1617 * sample_rate / 44100.0f),
        static_cast<int>(1491 * sample_rate / 44100.0f),
        static_cast<int>(1422 * sample_rate / 44100.0f)
    };
    
    // Allpass filter delays (in samples)
    const int allpass_delays[kNumAllpass_] = {
        static_cast<int>(225 * sample_rate / 44100.0f),
        static_cast<int>(556 * sample_rate / 44100.0f)
    };
    
    // Initialize comb filters
    for (int i = 0; i < kNumCombs_; ++i) {
        comb_buffers_[i].resize(comb_delays[i], 0.0f);
        comb_indices_[i] = 0;
        comb_feedback_[i] = 0.0f;
        comb_damp_[i] = 0.0f;
    }
    
    // Initialize allpass filters
    for (int i = 0; i < kNumAllpass_; ++i) {
        allpass_buffers_[i].resize(allpass_delays[i], 0.0f);
        allpass_indices_[i] = 0;
    }
    
    buffers_initialized_ = true;
    last_sample_rate_ = sample_rate;
}

bool ReverbNode::AddInput(AudioNode* input_node, int pin_id) {
    if (input_ != nullptr) {
        return false;
    }
    input_ = input_node;
    return true;
}

void ReverbNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node) {
        input_ = nullptr;
    }
}

std::vector<float> ReverbNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    InitializeBuffers(sample_rate);
    
    // Get input audio
    std::vector<float> input_audio = input_->GenerateAudio(num_samples, sample_rate);
    if (input_audio.empty()) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    std::vector<float> output(num_samples);
    
    // Calculate feedback based on room size
    float feedback = 0.28f + room_size_ * 0.7f;
    float damp_coeff = damping_;
    float damp_inv = 1.0f - damp_coeff;
    
    for (int i = 0; i < num_samples; ++i) {
        float input_sample = input_audio[i];
        float comb_sum = 0.0f;
        
        // Process comb filters (parallel)
        for (int c = 0; c < kNumCombs_; ++c) {
            int buffer_size = comb_buffers_[c].size();
            int& idx = comb_indices_[c];
            
            // Read from delay line
            float delayed = comb_buffers_[c][idx];
            
            // One-pole lowpass filter for damping
            comb_damp_[c] = delayed * damp_inv + comb_damp_[c] * damp_coeff;
            
            // Write to delay line with feedback
            comb_buffers_[c][idx] = input_sample + comb_damp_[c] * feedback;
            
            // Accumulate output
            comb_sum += delayed;
            
            // Advance index
            idx = (idx + 1) % buffer_size;
        }
        
        // Average the comb filter outputs
        float reverb_sample = comb_sum / kNumCombs_;
        
        // Process allpass filters (series)
        for (int a = 0; a < kNumAllpass_; ++a) {
            int buffer_size = allpass_buffers_[a].size();
            int& idx = allpass_indices_[a];
            
            float delayed = allpass_buffers_[a][idx];
            float allpass_feedback = 0.5f;
            
            // Allpass filter equation
            float allpass_out = -reverb_sample + delayed;
            allpass_buffers_[a][idx] = reverb_sample + delayed * allpass_feedback;
            reverb_sample = allpass_out;
            
            idx = (idx + 1) % buffer_size;
        }
        
        // Mix dry and wet signals
        output[i] = input_sample * dry_level_ + reverb_sample * wet_level_;
    }
    
    return output;
}

void ReverbNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Reverb %d", node_id_);
    ImGui::PushItemWidth(140.0f);
    
    ImGui::SliderFloat("Room Size", &room_size_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Damping", &damping_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Wet", &wet_level_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Dry", &dry_level_, 0.0f, 1.0f, "%.2f");
    
    ImGui::PopItemWidth();
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("<- Input");
    ed::EndPin();
    
    ImGui::SameLine();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Output ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

// ============================================================================
// PitchShifterNode Implementation
// ============================================================================

PitchShifterNode::PitchShifterNode(int node_id)
    : AudioNode(node_id, NodeType::kPitchShifter)
    , input_pin_id_(node_id * 1000 + 500)
    , input_(nullptr)
    , semitones_(0.0f)
    , write_index_(0)
    , phasor_(0.0) {
    delay_buffer_.resize(kBufferSize, 0.0f);
}

bool PitchShifterNode::AddInput(AudioNode* input_node, int pin_id) {
    if (input_ != nullptr) {
        return false;
    }
    input_ = input_node;
    return true;
}

void PitchShifterNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node) {
        input_ = nullptr;
    }
}

float PitchShifterNode::ReadBuffer(double index) {
    // Handle wrapping
    while (index < 0.0) index += kBufferSize;
    while (index >= kBufferSize) index -= kBufferSize;
    
    int idx0 = static_cast<int>(index);
    int idx1 = (idx0 + 1) % kBufferSize;
    float frac = index - idx0;
    
    return delay_buffer_[idx0] * (1.0f - frac) + delay_buffer_[idx1] * frac;
}

std::vector<float> PitchShifterNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // Get input audio
    std::vector<float> input_audio = input_->GenerateAudio(num_samples, sample_rate);
    if (input_audio.empty()) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    std::vector<float> output(num_samples);
    
    float pitch_ratio = std::pow(2.0f, semitones_ / 12.0f);
    int window_samples = static_cast<int>(kWindowSize * sample_rate);
    
    // Rate at which the delay time changes
    double phasor_inc = (1.0 - pitch_ratio) / window_samples;
    
    for (int i = 0; i < num_samples; ++i) {
        float input_sample = input_audio[i];
        
        // Write to buffer
        delay_buffer_[write_index_] = input_sample;
        
        // Calculate read positions based on phasor
        // Delay varies from 0 to window_samples
        double delay_a = phasor_ * window_samples;
        double delay_b = std::fmod(phasor_ + 0.5, 1.0) * window_samples;
        
        // Read from buffer
        float sample_a = ReadBuffer(write_index_ - delay_a);
        float sample_b = ReadBuffer(write_index_ - delay_b);
        
        // Triangle windowing
        float gain_a = (phasor_ < 0.5) ? (phasor_ * 2.0f) : ((1.0f - phasor_) * 2.0f);
        float gain_b = (std::fmod(phasor_ + 0.5, 1.0) < 0.5) ? 
                       (std::fmod(phasor_ + 0.5, 1.0) * 2.0f) : 
                       ((1.0f - std::fmod(phasor_ + 0.5, 1.0)) * 2.0f);
        
        output[i] = sample_a * gain_a + sample_b * gain_b;
        
        // Increment write index
        write_index_ = (write_index_ + 1) % kBufferSize;
        
        // Increment phasor
        phasor_ += phasor_inc;
        if (phasor_ >= 1.0) phasor_ -= 1.0;
        if (phasor_ < 0.0) phasor_ += 1.0;
    }
    
    return output;
}

void PitchShifterNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Pitch Shifter %d", node_id_);
    ImGui::PushItemWidth(140.0f);
    
    // Instead, step by 1 unit
    ImGui::SliderFloat("Semitones", &semitones_, -12.0f, 12.0f, "%1.f");

    ImGui::PopItemWidth();
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("<- Input");
    ed::EndPin();
    
    ImGui::SameLine();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Output ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

} // namespace audio_nodes
