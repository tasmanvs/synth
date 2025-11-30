#include "audio/nodes/source_node.h"
#include "audio/nodes/string_resonator.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include <cmath>
#include <algorithm>
#include <complex>

namespace audio_nodes {

SourceNode::SourceNode(int node_id)
    : AudioNode(node_id, NodeType::kSource)
    , frequency_(440.0f)
    , volume_(0.5f)
    , waveform_type_(WaveformType::kSine)
    , last_frequency_(440.0f)
    , last_volume_(0.5f)
    , last_waveform_type_(WaveformType::kSine)
    , parameters_changed_(false)
    , phases_(1, 0.0)
    , num_harmonics_(8)
    , smoothing_time_(1.0f)
    , frequency_end_(440.0f)
    , frequency_count_(1)
    , last_frequency_end_(440.0f)
    , last_frequency_count_(1)
    , end_frequency_inclusive_(true)
    , input_pin_id_start_(node_id * 100 + 2)
    , input_pin_id_end_(node_id * 100 + 3)
    , input_start_(nullptr)
    , input_end_(nullptr) {
    buffer_config_.frequency_hz = frequency_;
    buffer_config_.amplitude = volume_;
    buffer_config_.sample_rate = 48000;
    buffer_config_.frame_count = 512;
}

bool SourceNode::AddInput(AudioNode* input_node, int pin_id) {
    if (pin_id == input_pin_id_start_) {
        input_start_ = input_node;
        return true;
    } else if (pin_id == input_pin_id_end_) {
        input_end_ = input_node;
        return true;
    }
    return false;
}

void SourceNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_start_ == input_node && pin_id == input_pin_id_start_) {
        input_start_ = nullptr;
    } else if (input_end_ == input_node && pin_id == input_pin_id_end_) {
        input_end_ = nullptr;
    }
}

std::vector<float> SourceNode::GenerateAudio(int num_samples, int sample_rate) {
    if (num_samples <= 0 || sample_rate <= 0) {
        return {};
    }

    switch (waveform_type_) {
        case WaveformType::kSine:
            return GenerateSine(num_samples, sample_rate);
        case WaveformType::kSawtooth:
            return GenerateSawtooth(num_samples, sample_rate);
        case WaveformType::kSquare:
            return GenerateSquare(num_samples, sample_rate);
        case WaveformType::kSmoothedSquare:
            return GenerateSmoothedSquare(num_samples, sample_rate);
        case WaveformType::kStringResonator:
            return GenerateStringResonator(num_samples, sample_rate);
        default:
            return GenerateSine(num_samples, sample_rate);
    }
}

std::vector<float> SourceNode::GenerateSine(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    const double pi = 3.14159265358979323846;
    
    // If volume is 0, return silence
    if (volume_ == 0.0f) {
        return output;
    }
    
    std::vector<float> freq_start_input;
    std::vector<float> freq_end_input;
    if (input_start_) {
        freq_start_input = input_start_->GenerateAudio(num_samples, sample_rate);
    }
    if (input_end_) {
        freq_end_input = input_end_->GenerateAudio(num_samples, sample_rate);
    }

    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        double& phase = phases_[freq_idx];
        
        // Amplitude is divided by count to avoid clipping
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
            float freq_start = !freq_start_input.empty() ? freq_start_input[i] : frequency_;
            float freq_end = !freq_end_input.empty() ? freq_end_input[i] : frequency_end_;
            
            // If either frequency is 0, output is 0 for this sample
            if (freq_start == 0.0f || freq_end == 0.0f) {
                continue;
            }
            
            float freq;
            if (frequency_count_ == 1) {
                freq = freq_start;
            } else {
                // Linear interpolation from freq_start to freq_end
                float t;
                if (end_frequency_inclusive_) {
                    t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
                } else {
                    t = static_cast<float>(freq_idx) / frequency_count_;
                }
                freq = freq_start + t * (freq_end - freq_start);
            }

            double phase_increment = 2.0 * pi * freq / sample_rate;
            output[i] += amplitude * std::sin(phase);
            phase += phase_increment;
            
            // Wrap phase to avoid precision issues
            if (phase >= 2.0 * pi) {
                phase -= 2.0 * pi;
            }
        }
    }
    
    return output;
}

std::vector<float> SourceNode::GenerateSawtooth(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    const double pi = 3.14159265358979323846;
    
    // If volume is 0, return silence
    if (volume_ == 0.0f) {
        return output;
    }

    std::vector<float> freq_start_input;
    std::vector<float> freq_end_input;
    if (input_start_) {
        freq_start_input = input_start_->GenerateAudio(num_samples, sample_rate);
    }
    if (input_end_) {
        freq_end_input = input_end_->GenerateAudio(num_samples, sample_rate);
    }

    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        double& phase = phases_[freq_idx];
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
            float freq_start = !freq_start_input.empty() ? freq_start_input[i] : frequency_;
            float freq_end = !freq_end_input.empty() ? freq_end_input[i] : frequency_end_;
            
            // If either frequency is 0, output is 0 for this sample
            if (freq_start == 0.0f || freq_end == 0.0f) {
                continue;
            }
            
            float freq;
            if (frequency_count_ == 1) {
                freq = freq_start;
            } else {
                // Linear interpolation from freq_start to freq_end
                float t;
                if (end_frequency_inclusive_) {
                    t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
                } else {
                    t = static_cast<float>(freq_idx) / frequency_count_;
                }
                freq = freq_start + t * (freq_end - freq_start);
            }            double phase_increment = 2.0 * pi * freq / sample_rate;
            
            // Sawtooth: ramps from -1 to 1 linearly
            double normalized_phase = phase / (2.0 * pi);
            output[i] += amplitude * (2.0f * normalized_phase - 1.0f);
            phase += phase_increment;
            
            // Wrap phase
            if (phase >= 2.0 * pi) {
                phase -= 2.0 * pi;
            }
        }
    }
    
    return output;
}

std::vector<float> SourceNode::GenerateSquare(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    const double pi = 3.14159265358979323846;
    
    // If volume is 0, return silence
    if (volume_ == 0.0f) {
        return output;
    }
    
    std::vector<float> freq_start_input;
    std::vector<float> freq_end_input;
    if (input_start_) {
        freq_start_input = input_start_->GenerateAudio(num_samples, sample_rate);
    }
    if (input_end_) {
        freq_end_input = input_end_->GenerateAudio(num_samples, sample_rate);
    }

    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        double& phase = phases_[freq_idx];
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
            float freq_start = !freq_start_input.empty() ? freq_start_input[i] : frequency_;
            float freq_end = !freq_end_input.empty() ? freq_end_input[i] : frequency_end_;
            
            // If either frequency is 0, output is 0 for this sample
            if (freq_start == 0.0f || freq_end == 0.0f) {
                continue;
            }
            
            float freq;
            if (frequency_count_ == 1) {
                freq = freq_start;
            } else {
                // Linear interpolation from freq_start to freq_end
                float t;
                if (end_frequency_inclusive_) {
                    t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
                } else {
                    t = static_cast<float>(freq_idx) / frequency_count_;
                }
                freq = freq_start + t * (freq_end - freq_start);
            }

            double phase_increment = 2.0 * pi * freq / sample_rate;
            
            // Square: +1 or -1 depending on which half of cycle
            output[i] += amplitude * (phase < pi ? 1.0f : -1.0f);
            phase += phase_increment;
            
            // Wrap phase
            if (phase >= 2.0 * pi) {
                phase -= 2.0 * pi;
            }
        }
    }
    
    return output;
}

std::vector<float> SourceNode::GenerateSmoothedSquare(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    const double pi = 3.14159265358979323846;
    
    // If volume is 0, return silence
    if (volume_ == 0.0f) {
        return output;
    }
    
    std::vector<float> freq_start_input;
    std::vector<float> freq_end_input;
    if (input_start_) {
        freq_start_input = input_start_->GenerateAudio(num_samples, sample_rate);
    }
    if (input_end_) {
        freq_end_input = input_end_->GenerateAudio(num_samples, sample_rate);
    }

    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        double& phase = phases_[freq_idx];
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
            float freq_start = !freq_start_input.empty() ? freq_start_input[i] : frequency_;
            float freq_end = !freq_end_input.empty() ? freq_end_input[i] : frequency_end_;
            
            // If either frequency is 0, output is 0 for this sample
            if (freq_start == 0.0f || freq_end == 0.0f) {
                continue;
            }
            
            float freq;
            if (frequency_count_ == 1) {
                freq = freq_start;
            } else {
                // Linear interpolation from freq_start to freq_end
                float t;
                if (end_frequency_inclusive_) {
                    t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
                } else {
                    t = static_cast<float>(freq_idx) / frequency_count_;
                }
                freq = freq_start + t * (freq_end - freq_start);
            }

            double phase_increment = 2.0 * pi * freq / sample_rate;
            
            // Calculate smoothing duration in radians  
            float smoothing_samples = (smoothing_time_ / 1000.0f) * sample_rate;
            smoothing_samples = std::max(2.0f, smoothing_samples);
            double smoothing_phase = smoothing_samples * phase_increment;
            smoothing_phase = std::min(smoothing_phase, pi * 0.48); // Max 48% of half-period
            
            // Normalize phase to [0, 2*pi)
            double norm_phase = std::fmod(phase, 2.0 * pi);
            if (norm_phase < 0) norm_phase += 2.0 * pi;
            
            float value;
            
            // The rising edge wraps around 0/2π, so we need to handle it specially
            // Check distance from phase=0 (accounting for wrap)
            double dist_from_zero = (norm_phase < pi) ? norm_phase : (2.0 * pi - norm_phase);
            
            if (dist_from_zero < smoothing_phase) {
                // Rising edge centered at phase=0 (wraps around 2π): -1 to +1
                // Map phase to transition parameter t ∈ [0, 1]
                double t;
                if (norm_phase <= smoothing_phase) {
                    // First part: [0, smoothing_phase]
                    t = 0.5 + norm_phase / (2.0 * smoothing_phase);
                } else {
                    // Wrapped part: [2π - smoothing_phase, 2π]
                    t = (norm_phase - (2.0 * pi - smoothing_phase)) / (2.0 * smoothing_phase);
                }
                t = std::max(0.0, std::min(1.0, t));
                float smooth_t = t * t * (3.0 - 2.0 * t);
                value = -1.0f + 2.0f * smooth_t;
            } else if (norm_phase < pi - smoothing_phase) {
                // High plateau
                value = 1.0f;
            } else if (norm_phase < pi + smoothing_phase) {
                // Falling edge centered at phase=π: +1 to -1
                double t = (norm_phase - (pi - smoothing_phase)) / (2.0 * smoothing_phase);
                t = std::max(0.0, std::min(1.0, t));
                float smooth_t = t * t * (3.0 - 2.0 * t);
                value = 1.0f - 2.0f * smooth_t;
            } else {
                // Low plateau
                value = -1.0f;
            }
            
            output[i] += amplitude * value;
            phase += phase_increment;
            
            // Wrap phase
            while (phase >= 2.0 * pi) {
                phase -= 2.0 * pi;
            }
            while (phase < 0.0) {
                phase += 2.0 * pi;
            }
        }
    }
    
    return output;
}

std::vector<float> SourceNode::GenerateStringResonator(int num_samples, int sample_rate) {
    std::vector<float> output(num_samples, 0.0f);
    
    // If volume is 0, return silence
    if (volume_ == 0.0f) {
        return output;
    }
    
    // Ensure phases vector matches frequency count * harmonics
    int total_oscillators = frequency_count_ * num_harmonics_;
    if (static_cast<int>(phases_.size()) != total_oscillators) {
        phases_.resize(total_oscillators, 0.0);
    }

    std::vector<float> freq_start_input;
    std::vector<float> freq_end_input;
    if (input_start_) {
        freq_start_input = input_start_->GenerateAudio(num_samples, sample_rate);
    }
    if (input_end_) {
        freq_end_input = input_end_->GenerateAudio(num_samples, sample_rate);
    }
    
    // Generate each base frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        // Pre-calculate base frequencies for this block
        std::vector<float> base_freqs(num_samples);
        for (int i = 0; i < num_samples; ++i) {
            float freq_start = !freq_start_input.empty() ? freq_start_input[i] : frequency_;
            float freq_end = !freq_end_input.empty() ? freq_end_input[i] : frequency_end_;
            
            // If either frequency is 0, output is 0 for this sample
            if (freq_start == 0.0f || freq_end == 0.0f) {
                base_freqs[i] = 0.0f;
                continue;
            }
            
            if (frequency_count_ == 1) {
                base_freqs[i] = freq_start;
            } else {
                float t;
                if (end_frequency_inclusive_) {
                    t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
                } else {
                    t = static_cast<float>(freq_idx) / frequency_count_;
                }
                base_freqs[i] = freq_start + t * (freq_end - freq_start);
            }
        }

        // Get phases for this frequency's harmonics
        std::vector<double> harmonic_phases(num_harmonics_);
        for (int h = 0; h < num_harmonics_; ++h) {
            harmonic_phases[h] = phases_[freq_idx * num_harmonics_ + h];
        }
        
        // Use library function to generate string resonator
        float volume_per_freq = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        audio_nodes::GenerateStringResonatorWithModulation(
            base_freqs, num_harmonics_, volume_per_freq, 
            num_samples, sample_rate, harmonic_phases, output);
        
        // Store updated phases
        for (int h = 0; h < num_harmonics_; ++h) {
            phases_[freq_idx * num_harmonics_ + h] = harmonic_phases[h];
        }
    }
    
    // Normalize output to prevent clipping
    float normalization = 1.0f / std::sqrt(static_cast<float>(num_harmonics_));
    for (int i = 0; i < num_samples; ++i) {
        output[i] *= normalization;
    }
    
    return output;
}

void SourceNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Source Node %d", node_id_);
    
    // Input pins for frequency modulation
    ed::BeginPin(input_pin_id_start_, ed::PinKind::Input);
    ImGui::Text("-> Freq Start");
    ed::EndPin();
    
    ed::BeginPin(input_pin_id_end_, ed::PinKind::Input);
    ImGui::Text("-> Freq End");
    ed::EndPin();

    ImGui::PushItemWidth(120.0f);
    
    // Waveform type selector
    ImGui::Text("Waveform:");
    int current_waveform = static_cast<int>(waveform_type_);
    if (ImGui::RadioButton("Sine", &current_waveform, static_cast<int>(WaveformType::kSine))) {
        waveform_type_ = WaveformType::kSine;
        parameters_changed_ = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Sawtooth", &current_waveform, static_cast<int>(WaveformType::kSawtooth))) {
        waveform_type_ = WaveformType::kSawtooth;
        parameters_changed_ = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Square", &current_waveform, static_cast<int>(WaveformType::kSquare))) {
        waveform_type_ = WaveformType::kSquare;
        parameters_changed_ = true;
    }
    if (ImGui::RadioButton("Smooth Sq", &current_waveform, static_cast<int>(WaveformType::kSmoothedSquare))) {
        waveform_type_ = WaveformType::kSmoothedSquare;
        parameters_changed_ = true;
    }
    if (ImGui::RadioButton("String", &current_waveform, static_cast<int>(WaveformType::kStringResonator))) {
        waveform_type_ = WaveformType::kStringResonator;
        parameters_changed_ = true;
    }
    
    // Disable sliders if inputs are connected
    if (input_start_) {
        ImGui::BeginDisabled();
    }
    if (ImGui::SliderFloat("Freq Start", &frequency_, 0.1f, 20000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (input_start_) {
        ImGui::EndDisabled();
    }
    
    if (input_end_) {
        ImGui::BeginDisabled();
    }
    if (ImGui::SliderFloat("Freq End", &frequency_end_, 0.1f, 20000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (input_end_) {
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Incl", &end_frequency_inclusive_);
    if (ImGui::SliderInt("Count", &frequency_count_, 1, 64)) {
        parameters_changed_ = true;
        phases_.resize(frequency_count_, 0.0);
    }
    if (ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f")) {
        parameters_changed_ = true;
    }
    
    // Show harmonics slider only for string resonator
    if (waveform_type_ == WaveformType::kStringResonator) {
        if (ImGui::SliderInt("Harmonics", &num_harmonics_, 1, 64)) {
            parameters_changed_ = true;
        }
    }
    
    // Show smoothing time slider only for smoothed square
    if (waveform_type_ == WaveformType::kSmoothedSquare) {
        if (ImGui::SliderFloat("Smoothing (ms)", &smoothing_time_, 0.1f, 10.0f, "%.1f")) {
            parameters_changed_ = true;
        }
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
    if (frequency_ != last_frequency_ || volume_ != last_volume_ || waveform_type_ != last_waveform_type_ ||
        frequency_end_ != last_frequency_end_ || frequency_count_ != last_frequency_count_) {
        last_frequency_ = frequency_;
        last_volume_ = volume_;
        last_waveform_type_ = waveform_type_;
        last_frequency_end_ = frequency_end_;
        last_frequency_count_ = frequency_count_;
        return true;
    }
    return parameters_changed_;
}

} // namespace audio_nodes
