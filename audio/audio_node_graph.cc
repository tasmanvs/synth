#include "audio/audio_node_graph.h"
#include "absl/log/log.h"
#include "pocketfft_hdronly.h"
#include <algorithm>
#include <complex>
#include "imgui.h"
#include "implot.h"

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
    , last_frequency_count_(1) {
    buffer_config_.frequency_hz = frequency_;
    buffer_config_.amplitude = volume_;
    buffer_config_.sample_rate = 48000;
    buffer_config_.frame_count = 512;
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
    
    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        float freq;
        if (frequency_count_ == 1) {
            freq = frequency_;
        } else {
            // Linear interpolation from frequency_ to frequency_end_
            float t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
            freq = frequency_ + t * (frequency_end_ - frequency_);
        }
        
        double phase_increment = 2.0 * pi * freq / sample_rate;
        double& phase = phases_[freq_idx];
        
        // Amplitude is divided by count to avoid clipping
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
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
    
    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        float freq;
        if (frequency_count_ == 1) {
            freq = frequency_;
        } else {
            float t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
            freq = frequency_ + t * (frequency_end_ - frequency_);
        }
        
        double phase_increment = 2.0 * pi * freq / sample_rate;
        double& phase = phases_[freq_idx];
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
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
    
    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        float freq;
        if (frequency_count_ == 1) {
            freq = frequency_;
        } else {
            float t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
            freq = frequency_ + t * (frequency_end_ - frequency_);
        }
        
        double phase_increment = 2.0 * pi * freq / sample_rate;
        double& phase = phases_[freq_idx];
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        for (int i = 0; i < num_samples; ++i) {
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
    
    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        float freq;
        if (frequency_count_ == 1) {
            freq = frequency_;
        } else {
            float t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
            freq = frequency_ + t * (frequency_end_ - frequency_);
        }
        
        double phase_increment = 2.0 * pi * freq / sample_rate;
        
        double& phase = phases_[freq_idx];
        float amplitude = volume_ / std::sqrt(static_cast<float>(frequency_count_));
        
        // Calculate smoothing duration in radians  
        float smoothing_samples = (smoothing_time_ / 1000.0f) * sample_rate;
        smoothing_samples = std::max(2.0f, smoothing_samples);
        double smoothing_phase = smoothing_samples * phase_increment;
        smoothing_phase = std::min(smoothing_phase, pi * 0.48); // Max 48% of half-period
        
        for (int i = 0; i < num_samples; ++i) {
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
    const double pi = 3.14159265358979323846;
    
    // Ensure phases vector matches frequency count
    if (static_cast<int>(phases_.size()) != frequency_count_) {
        phases_.resize(frequency_count_, 0.0);
    }
    
    // Generate each base frequency
    for (int freq_idx = 0; freq_idx < frequency_count_; ++freq_idx) {
        float base_freq;
        if (frequency_count_ == 1) {
            base_freq = frequency_;
        } else {
            float t = static_cast<float>(freq_idx) / (frequency_count_ - 1);
            base_freq = frequency_ + t * (frequency_end_ - frequency_);
        }
        
        double& phase = phases_[freq_idx];
        
        // Generate harmonics at f, f/2, f/3, f/4, etc.
        for (int harmonic = 1; harmonic <= num_harmonics_; ++harmonic) {
            double harmonic_freq = base_freq / static_cast<double>(harmonic);
            double phase_increment = 2.0 * pi * harmonic_freq / sample_rate;
            double harmonic_phase = phase;
            
            // Amplitude decreases with higher harmonics (1/n falloff)
            float harmonic_amplitude = volume_ / (static_cast<float>(harmonic) * std::sqrt(static_cast<float>(frequency_count_)));
            
            for (int i = 0; i < num_samples; ++i) {
                output[i] += harmonic_amplitude * std::sin(harmonic_phase);
                harmonic_phase += phase_increment;
                
                // Wrap phase
                if (harmonic_phase >= 2.0 * pi) {
                    harmonic_phase -= 2.0 * pi;
                }
            }
        }
        
        // Update phase for next call
        double phase_increment = 2.0 * pi * base_freq / sample_rate;
        phase += phase_increment * num_samples;
        while (phase >= 2.0 * pi) {
            phase -= 2.0 * pi;
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
    
    if (ImGui::SliderFloat("Freq Start", &frequency_, 0.1f, 20000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderFloat("Freq End", &frequency_end_, 0.1f, 20000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderInt("Count", &frequency_count_, 1, 64)) {
        parameters_changed_ = true;
        phases_.resize(frequency_count_, 0.0);
    }
    if (ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f")) {
        parameters_changed_ = true;
    }
    
    // Show harmonics slider only for string resonator
    if (waveform_type_ == WaveformType::kStringResonator) {
        if (ImGui::SliderInt("Harmonics", &num_harmonics_, 1, 16)) {
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

// ============================================================================
// HarmonicNode Implementation
// ============================================================================

HarmonicNode::HarmonicNode(int node_id)
    : AudioNode(node_id, NodeType::kHarmonic)
    , base_frequency_(220.0f)
    , volume_(0.3f)
    , num_harmonics_(5)
    , last_base_frequency_(220.0f)
    , last_volume_(0.3f)
    , last_num_harmonics_(5)
    , parameters_changed_(false) {
    UpdateHarmonics();
}

void HarmonicNode::UpdateHarmonics() {
    // Preserve existing generators to maintain phase continuity
    int current_count = static_cast<int>(harmonic_generators_.size());
    
    if (num_harmonics_ > current_count) {
        // Add new generators
        for (int i = current_count; i < num_harmonics_; ++i) {
            audio_loop::BufferConfig config;
            config.frequency_hz = base_frequency_ * (i + 1);
            config.amplitude = volume_ / static_cast<float>(num_harmonics_);
            config.sample_rate = 48000;
            config.frame_count = 512;
            
            harmonic_configs_.push_back(config);
            harmonic_generators_.emplace_back(0.0f);
        }
    } else if (num_harmonics_ < current_count) {
        // Remove excess generators
        harmonic_configs_.resize(num_harmonics_);
        harmonic_generators_.resize(num_harmonics_);
    }
    
    // Update existing configs (generators maintain their phase)
    for (int i = 0; i < num_harmonics_; ++i) {
        if (i < static_cast<int>(harmonic_configs_.size())) {
            harmonic_configs_[i].frequency_hz = base_frequency_ * (i + 1);
            harmonic_configs_[i].amplitude = volume_ / static_cast<float>(num_harmonics_);
        }
    }
}

std::vector<float> HarmonicNode::GenerateAudio(int num_samples, int sample_rate) {
    if (num_samples <= 0 || sample_rate <= 0) {
        return {};
    }
    
    std::vector<float> output(num_samples, 0.0f);
    
    // Generate and sum all harmonics
    for (size_t i = 0; i < harmonic_generators_.size(); ++i) {
        harmonic_configs_[i].frame_count = num_samples;
        harmonic_configs_[i].sample_rate = sample_rate;
        harmonic_configs_[i].frequency_hz = base_frequency_ * (i + 1);
        harmonic_configs_[i].amplitude = volume_ / static_cast<float>(num_harmonics_);
        
        auto harmonic_buffer = harmonic_generators_[i].GenerateBuffer(harmonic_configs_[i]);
        
        for (int j = 0; j < num_samples && j < static_cast<int>(harmonic_buffer.size()); ++j) {
            output[j] += harmonic_buffer[j];
        }
    }
    
    // Clamp to prevent overflow
    for (int i = 0; i < num_samples; ++i) {
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
    }
    
    return output;
}

void HarmonicNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Harmonic Node %d", node_id_);
    ImGui::PushItemWidth(120.0f);
    if (ImGui::SliderFloat("Base Freq", &base_frequency_, 20.0f, 1000.0f, "%.1f Hz")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f, "%.2f")) {
        parameters_changed_ = true;
    }
    if (ImGui::SliderInt("Harmonics", &num_harmonics_, 1, 16)) {
        parameters_changed_ = true;
        UpdateHarmonics();
    }
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

bool HarmonicNode::HasParametersChanged() {
    if (base_frequency_ != last_base_frequency_ || 
        volume_ != last_volume_ || 
        num_harmonics_ != last_num_harmonics_) {
        last_base_frequency_ = base_frequency_;
        last_volume_ = volume_;
        last_num_harmonics_ = num_harmonics_;
        return true;
    }
    return parameters_changed_;
}

// ============================================================================
// LowpassFilterNode Implementation
// ============================================================================

LowpassFilterNode::LowpassFilterNode(int node_id)
    : AudioNode(node_id, NodeType::kLowpassFilter)
    , input_pin_id_(node_id * 1000 + 501)
    , input_(nullptr)
    , cutoff_frequency_(1000.0f)
    , filter_order_(8)
    , show_bode_plot_(false) {
    ResizeFilterArrays();
    UpdateFilterCoefficients(48000); // Initialize with default sample rate
}

void LowpassFilterNode::ResizeFilterArrays() {
    // Calculate number of cascaded biquad sections needed
    int num_sections = (filter_order_ + 1) / 2;
    
    b0_.resize(num_sections);
    b1_.resize(num_sections);
    b2_.resize(num_sections);
    a1_.resize(num_sections);
    a2_.resize(num_sections);
    
    x1_.resize(num_sections, 0.0f);
    x2_.resize(num_sections, 0.0f);
    y1_.resize(num_sections, 0.0f);
    y2_.resize(num_sections, 0.0f);
}

void LowpassFilterNode::UpdateFilterCoefficients(int sample_rate) {
    // Prevent invalid frequencies
    if (cutoff_frequency_ <= 0.0f || cutoff_frequency_ >= sample_rate / 2.0f) {
        cutoff_frequency_ = std::min(std::max(cutoff_frequency_, 1.0f), sample_rate / 2.0f - 1.0f);
    }
    
    ResizeFilterArrays();
    
    // Compute analog poles for Butterworth filter
    const float pi = 3.14159265358979323846f;
    int num_sections = (filter_order_ + 1) / 2;
    
    // Prewarp cutoff frequency for bilinear transform
    float wc = 2.0f * pi * cutoff_frequency_;
    float T = 1.0f / sample_rate;
    float K = 2.0f / T;
    float K2 = K * K;
    
    for (int i = 0; i < num_sections; ++i) {
        // Compute pole angle for this section
        float theta;
        if (filter_order_ % 2 == 1 && i == 0) {
            // First section for odd order is first-order
            theta = pi;
        } else {
            int section_idx = (filter_order_ % 2 == 1) ? i : i;
            int pole_idx = section_idx + (filter_order_ % 2 == 1 ? 0 : 1);
            theta = pi * (2.0f * pole_idx + filter_order_ - 1) / (2.0f * filter_order_);
        }
        
        // Analog pole location (normalized to unit circle)
        float sigma = std::cos(theta);
        float omega = std::sin(theta);
        
        // Scale by cutoff frequency
        float ps = sigma * wc;
        float po = omega * wc;
        
        // Bilinear transform
        if (filter_order_ % 2 == 1 && i == 0) {
            // First-order section for odd orders
            float b_0 = wc;
            float b_1 = wc;
            float a_0 = K - ps;
            float a_1 = -(K + ps);
            
            b0_[i] = b_0 / a_0;
            b1_[i] = b_1 / a_0;
            b2_[i] = 0.0f;
            a1_[i] = a_1 / a_0;
            a2_[i] = 0.0f;
        } else {
            // Second-order section
            float ps2_po2 = ps * ps + po * po;
            
            float b_0 = wc * wc;
            float b_1 = 2.0f * wc * wc;
            float b_2 = wc * wc;
            
            float a_0 = K2 - 2.0f * K * ps + ps2_po2;
            float a_1 = 2.0f * ps2_po2 - 2.0f * K2;
            float a_2 = K2 + 2.0f * K * ps + ps2_po2;
            
            b0_[i] = b_0 / a_0;
            b1_[i] = b_1 / a_0;
            b2_[i] = b_2 / a_0;
            a1_[i] = a_1 / a_0;
            a2_[i] = a_2 / a_0;
        }
    }
}

float LowpassFilterNode::ProcessSample(float input) {
    float output = input;
    
    // Process through each biquad section
    for (size_t i = 0; i < b0_.size(); ++i) {
        float x = output;
        output = b0_[i] * x + b1_[i] * x1_[i] + b2_[i] * x2_[i] 
                 - a1_[i] * y1_[i] - a2_[i] * y2_[i];
        
        // Update state
        x2_[i] = x1_[i];
        x1_[i] = x;
        y2_[i] = y1_[i];
        y1_[i] = output;
    }
    
    return output;
}

float LowpassFilterNode::ComputeFrequencyResponse(float frequency, int sample_rate) {
    const float pi = 3.14159265358979323846f;
    float omega = 2.0f * pi * frequency / sample_rate;
    
    std::complex<float> H(1.0f, 0.0f);
    std::complex<float> z = std::exp(std::complex<float>(0.0f, omega));
    std::complex<float> z_inv = 1.0f / z;
    std::complex<float> z_inv2 = z_inv * z_inv;
    
    // Multiply transfer function of each biquad section
    for (size_t i = 0; i < b0_.size(); ++i) {
        std::complex<float> num = b0_[i] + b1_[i] * z_inv + b2_[i] * z_inv2;
        std::complex<float> den = 1.0f + a1_[i] * z_inv + a2_[i] * z_inv2;
        H *= num / den;
    }
    
    float magnitude = std::abs(H);
    return 20.0f * std::log10(std::max(magnitude, 1e-10f));
}

std::vector<float> LowpassFilterNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // Update filter coefficients if needed (checks internally)
    if (b0_.empty() || b0_.size() != static_cast<size_t>((filter_order_ + 1) / 2)) {
        UpdateFilterCoefficients(sample_rate);
    }
    
    auto input_data = input_->GenerateAudio(num_samples, sample_rate);
    std::vector<float> output(num_samples);
    
    // Process samples through cascaded biquad filter
    for (int i = 0; i < num_samples; ++i) {
        output[i] = ProcessSample(input_data[i]);
    }
    
    return output;
}

void LowpassFilterNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Lowpass Filter %d", node_id_);
    
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    
    ImGui::PushItemWidth(120.0f);
    
    bool params_changed = false;
    if (ImGui::SliderFloat("Cutoff", &cutoff_frequency_, 20.0f, 20000.0f, "%.1f Hz", ImGuiSliderFlags_Logarithmic)) {
        params_changed = true;
    }
    
    int prev_order = filter_order_;
    if (ImGui::SliderInt("Order", &filter_order_, 1, 64)) {
        if (filter_order_ != prev_order) {
            params_changed = true;
        }
    }
    ImGui::PopItemWidth();
    
    // Update filter coefficients if parameters changed
    if (params_changed) {
        // Coefficients will be regenerated in GenerateAudio with actual sample rate
        b0_.clear(); // Force recalculation
    }
    
    if (ImGui::Button("Show Bode Plot")) {
        show_bode_plot_ = true;
    }
    
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    
    ImGui::PopID();
    
    // Draw bode plot in separate window
    if (show_bode_plot_) {
        ed::Suspend();
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        char window_name[64];
        snprintf(window_name, sizeof(window_name), "Lowpass Bode Plot ##%d", node_id_);
        if (ImGui::Begin(window_name, &show_bode_plot_, ImGuiWindowFlags_None)) {
            const int num_points = 100;
            static float freq_data[100];
            static float plot_data[100];
            const int sample_rate = 48000;
            
            // Ensure filter coefficients are up to date
            if (b0_.empty() || b0_.size() != static_cast<size_t>((filter_order_ + 1) / 2)) {
                UpdateFilterCoefficients(sample_rate);
            }
            
            for (int i = 0; i < num_points; ++i) {
                float freq = 20.0f * std::pow(20000.0f / 20.0f, static_cast<float>(i) / (num_points - 1));
                freq_data[i] = freq;
                plot_data[i] = ComputeFrequencyResponse(freq, sample_rate);
            }
            
            if (ImPlot::BeginPlot("Frequency Response", ImVec2(-1, -1))) {
                ImPlot::SetupAxes("Frequency (Hz)", "Magnitude (dB)");
                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
                ImPlot::SetupAxesLimits(20, 20000, -80, 10, ImPlotCond_Once);
                ImPlot::PlotLine("Response", freq_data, plot_data, num_points);
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
        ed::Resume();
    }
}

bool LowpassFilterNode::AddInput(AudioNode* input_node, int pin_id) {
    if (pin_id != input_pin_id_) return false;
    input_ = input_node;
    return true;
}

void LowpassFilterNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node && pin_id == input_pin_id_) {
        input_ = nullptr;
    }
}

// ============================================================================
// HighpassFilterNode Implementation
// ============================================================================

HighpassFilterNode::HighpassFilterNode(int node_id)
    : AudioNode(node_id, NodeType::kHighpassFilter)
    , input_pin_id_(node_id * 1000 + 502)
    , input_(nullptr)
    , cutoff_frequency_(1000.0f)
    , filter_order_(8)
    , show_bode_plot_(false) {
    ResizeFilterArrays();
    UpdateFilterCoefficients(48000); // Initialize with default sample rate
}

void HighpassFilterNode::ResizeFilterArrays() {
    // Calculate number of cascaded biquad sections needed
    int num_sections = (filter_order_ + 1) / 2;
    
    b0_.resize(num_sections);
    b1_.resize(num_sections);
    b2_.resize(num_sections);
    a1_.resize(num_sections);
    a2_.resize(num_sections);
    
    x1_.resize(num_sections, 0.0f);
    x2_.resize(num_sections, 0.0f);
    y1_.resize(num_sections, 0.0f);
    y2_.resize(num_sections, 0.0f);
}

void HighpassFilterNode::UpdateFilterCoefficients(int sample_rate) {
    // Prevent invalid frequencies
    if (cutoff_frequency_ <= 0.0f || cutoff_frequency_ >= sample_rate / 2.0f) {
        cutoff_frequency_ = std::min(std::max(cutoff_frequency_, 1.0f), sample_rate / 2.0f - 1.0f);
    }
    
    ResizeFilterArrays();
    
    // Compute analog poles for Butterworth filter
    const float pi = 3.14159265358979323846f;
    int num_sections = (filter_order_ + 1) / 2;
    
    // Prewarp cutoff frequency for bilinear transform
    float wc = 2.0f * pi * cutoff_frequency_;
    float T = 1.0f / sample_rate;
    float K = 2.0f / T;
    float K2 = K * K;
    
    for (int i = 0; i < num_sections; ++i) {
        // Compute pole angle for this section
        float theta;
        if (filter_order_ % 2 == 1 && i == 0) {
            // First section for odd order is first-order
            theta = pi;
        } else {
            int section_idx = (filter_order_ % 2 == 1) ? i : i;
            int pole_idx = section_idx + (filter_order_ % 2 == 1 ? 0 : 1);
            theta = pi * (2.0f * pole_idx + filter_order_ - 1) / (2.0f * filter_order_);
        }
        
        // Analog pole location (normalized to unit circle)
        float sigma = std::cos(theta);
        float omega = std::sin(theta);
        
        // Scale by cutoff frequency
        float ps = sigma * wc;
        float po = omega * wc;
        
        // Bilinear transform for highpass (lowpass to highpass transformation)
        if (filter_order_ % 2 == 1 && i == 0) {
            // First-order section for odd orders
            float b_0 = K;
            float b_1 = -K;
            float a_0 = K - ps;
            float a_1 = -(K + ps);
            
            b0_[i] = b_0 / a_0;
            b1_[i] = b_1 / a_0;
            b2_[i] = 0.0f;
            a1_[i] = a_1 / a_0;
            a2_[i] = 0.0f;
        } else {
            // Second-order section
            float ps2_po2 = ps * ps + po * po;
            
            float b_0 = K2;
            float b_1 = -2.0f * K2;
            float b_2 = K2;
            
            float a_0 = K2 - 2.0f * K * ps + ps2_po2;
            float a_1 = 2.0f * ps2_po2 - 2.0f * K2;
            float a_2 = K2 + 2.0f * K * ps + ps2_po2;
            
            b0_[i] = b_0 / a_0;
            b1_[i] = b_1 / a_0;
            b2_[i] = b_2 / a_0;
            a1_[i] = a_1 / a_0;
            a2_[i] = a_2 / a_0;
        }
    }
}

float HighpassFilterNode::ProcessSample(float input) {
    float output = input;
    
    // Process through each biquad section
    for (size_t i = 0; i < b0_.size(); ++i) {
        float x = output;
        output = b0_[i] * x + b1_[i] * x1_[i] + b2_[i] * x2_[i] 
                 - a1_[i] * y1_[i] - a2_[i] * y2_[i];
        
        // Update state
        x2_[i] = x1_[i];
        x1_[i] = x;
        y2_[i] = y1_[i];
        y1_[i] = output;
    }
    
    return output;
}

float HighpassFilterNode::ComputeFrequencyResponse(float frequency, int sample_rate) {
    const float pi = 3.14159265358979323846f;
    float omega = 2.0f * pi * frequency / sample_rate;
    
    std::complex<float> H(1.0f, 0.0f);
    std::complex<float> z = std::exp(std::complex<float>(0.0f, omega));
    std::complex<float> z_inv = 1.0f / z;
    std::complex<float> z_inv2 = z_inv * z_inv;
    
    // Multiply transfer function of each biquad section
    for (size_t i = 0; i < b0_.size(); ++i) {
        std::complex<float> num = b0_[i] + b1_[i] * z_inv + b2_[i] * z_inv2;
        std::complex<float> den = 1.0f + a1_[i] * z_inv + a2_[i] * z_inv2;
        H *= num / den;
    }
    
    float magnitude = std::abs(H);
    return 20.0f * std::log10(std::max(magnitude, 1e-10f));
}

std::vector<float> HighpassFilterNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    // Update filter coefficients if needed (checks internally)
    if (b0_.empty() || b0_.size() != static_cast<size_t>((filter_order_ + 1) / 2)) {
        UpdateFilterCoefficients(sample_rate);
    }
    
    auto input_data = input_->GenerateAudio(num_samples, sample_rate);
    std::vector<float> output(num_samples);
    
    // Process samples through cascaded biquad filter
    for (int i = 0; i < num_samples; ++i) {
        output[i] = ProcessSample(input_data[i]);
    }
    
    return output;
}

void HighpassFilterNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Highpass Filter %d", node_id_);
    
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    
    ImGui::PushItemWidth(120.0f);
    
    bool params_changed = false;
    if (ImGui::SliderFloat("Cutoff", &cutoff_frequency_, 20.0f, 20000.0f, "%.1f Hz", ImGuiSliderFlags_Logarithmic)) {
        params_changed = true;
    }
    
    int prev_order = filter_order_;
    if (ImGui::SliderInt("Order", &filter_order_, 1, 64)) {
        if (filter_order_ != prev_order) {
            params_changed = true;
        }
    }
    ImGui::PopItemWidth();
    
    // Update filter coefficients if parameters changed
    if (params_changed) {
        // Coefficients will be regenerated in GenerateAudio with actual sample rate
        b0_.clear(); // Force recalculation
    }
    
    if (ImGui::Button("Show Bode Plot")) {
        show_bode_plot_ = true;
    }
    
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    
    ImGui::PopID();
    
    // Draw bode plot in separate window
    if (show_bode_plot_) {
        ed::Suspend();
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        char window_name[64];
        snprintf(window_name, sizeof(window_name), "Highpass Bode Plot ##%d", node_id_);
        if (ImGui::Begin(window_name, &show_bode_plot_, ImGuiWindowFlags_None)) {
            const int num_points = 100;
            static float freq_data[100];
            static float plot_data[100];
            const int sample_rate = 48000;
            
            // Ensure filter coefficients are up to date
            if (b0_.empty() || b0_.size() != static_cast<size_t>((filter_order_ + 1) / 2)) {
                UpdateFilterCoefficients(sample_rate);
            }
            
            for (int i = 0; i < num_points; ++i) {
                float freq = 20.0f * std::pow(20000.0f / 20.0f, static_cast<float>(i) / (num_points - 1));
                freq_data[i] = freq;
                plot_data[i] = ComputeFrequencyResponse(freq, sample_rate);
            }
            
            if (ImPlot::BeginPlot("Frequency Response", ImVec2(-1, -1))) {
                ImPlot::SetupAxes("Frequency (Hz)", "Magnitude (dB)");
                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
                ImPlot::SetupAxesLimits(20, 20000, -80, 10, ImPlotCond_Once);
                ImPlot::PlotLine("Response", freq_data, plot_data, num_points);
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
        ed::Resume();
    }
}

bool HighpassFilterNode::AddInput(AudioNode* input_node, int pin_id) {
    if (pin_id != input_pin_id_) return false;
    input_ = input_node;
    return true;
}

void HighpassFilterNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node && pin_id == input_pin_id_) {
        input_ = nullptr;
    }
}

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
        
        // Smooth windowing (Hanning-like)
        // float gain_a = 0.5f * (1.0f - std::cos(2.0f * 3.14159f * phasor_));
        // float gain_b = 0.5f * (1.0f - std::cos(2.0f * 3.14159f * std::fmod(phasor_ + 0.5, 1.0)));
        
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
    
    // ImGui::SliderFloat("Semitones", &semitones_, -12.0f, 12.0f, "%.1f");

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

// ============================================================================
// BandpassFilterNode Implementation
// ============================================================================

BandpassFilterNode::BandpassFilterNode(int node_id)
    : AudioNode(node_id, NodeType::kBandpassFilter)
    , input_pin_id_(node_id * 1000 + 500)
    , input_(nullptr)
    , center_frequency_(1000.0f)
    , bandwidth_(200.0f)
    , b0_(0.0f), b1_(0.0f), b2_(0.0f), a1_(0.0f), a2_(0.0f)
    , x1_(0.0f), x2_(0.0f), y1_(0.0f), y2_(0.0f) {
}

void BandpassFilterNode::UpdateFilterCoefficients(int sample_rate) {
    // Biquad bandpass filter design
    float omega = 2.0f * 3.14159265359f * center_frequency_ / sample_rate;
    float alpha = std::sin(omega) * std::sinh(std::log(2.0f) / 2.0f * bandwidth_ * omega / std::sin(omega));
    
    float a0 = 1.0f + alpha;
    b0_ = alpha / a0;
    b1_ = 0.0f;
    b2_ = -alpha / a0;
    a1_ = -2.0f * std::cos(omega) / a0;
    a2_ = (1.0f - alpha) / a0;
}

float BandpassFilterNode::ProcessSample(float input) {
    float output = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
    
    // Update state
    x2_ = x1_;
    x1_ = input;
    y2_ = y1_;
    y1_ = output;
    
    return output;
}

std::vector<float> BandpassFilterNode::GenerateAudio(int num_samples, int sample_rate) {
    if (!input_ || num_samples <= 0) {
        return std::vector<float>(num_samples, 0.0f);
    }
    
    UpdateFilterCoefficients(sample_rate);
    
    auto input_data = input_->GenerateAudio(num_samples, sample_rate);
    std::vector<float> output(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        output[i] = ProcessSample(input_data[i]);
    }
    
    return output;
}

void BandpassFilterNode::Draw() {
    namespace ed = ax::NodeEditor;
    ImGui::PushID(node_id_);
    
    ed::BeginNode(node_id_);
    
    ImGui::Text("Bandpass Filter %d", node_id_);
    
    // Input pin
    ed::BeginPin(input_pin_id_, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    
    ImGui::PushItemWidth(120.0f);
    ImGui::SliderFloat("Center Freq", &center_frequency_, 20.0f, 20000.0f, "%.1f Hz", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Bandwidth", &bandwidth_, 10.0f, 5000.0f, "%.1f Hz", ImGuiSliderFlags_Logarithmic);
    ImGui::PopItemWidth();
    
    // Output pin
    ed::BeginPin(output_pin_id_, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    
    ed::EndNode();
    ImGui::PopID();
}

bool BandpassFilterNode::AddInput(AudioNode* input_node, int pin_id) {
    if (pin_id != input_pin_id_) return false;
    input_ = input_node;
    return true;
}

void BandpassFilterNode::RemoveInput(AudioNode* input_node, int pin_id) {
    if (input_ == input_node && pin_id == input_pin_id_) {
        input_ = nullptr;
    }
}

// ============================================================================
// SumNode Implementation
// ============================================================================

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
    if (!graph_) {
        return false;
    }

    int pin_id = node_id_ * 100 + 2 + next_pin_offset_;
    next_pin_offset_++;
    inputs_.push_back({pin_id, nullptr});
    graph_->RegisterPin(pin_id, node_id_);
    return true;
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
    , max_plot_samples_(16000)
    , pcm_convert_buffer_()
    , streaming_buffer_size_(64)
    , max_queue_buffers_(64)
    , needs_stream_prime_(true)
    , capture_buffer_()
    , capture_target_samples_(48000)
    , capture_samples_collected_(0)
    , capture_active_(false)
    , capture_ready_(false)
    , capture_target_input_(48000)
    , spectrogram_time_slices_(100)
    , fft_size_(512)
    , fft_input_buffer_(fft_size_, 0.0f)
    , sample_rate_(48000)
    , spectrogram_sample_counter_(0)
    , frequency_axis_min_(0.0)
    , frequency_axis_max_(24000.0) {
    // Initialize Hann window for FFT
    fft_window_.resize(fft_size_);
    for (int i = 0; i < fft_size_; ++i) {
        fft_window_[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265359f * i / (fft_size_ - 1)));
    }
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
    if (ImGui::SliderFloat("Master", &volume_, 0.0f, 1.0f, "%.2f")) {
        if (audio_interface_ && playing_) {
            audio_interface_->SetVolume(volume_);
        }
    }
    ImGui::PopItemWidth();

    if (audio_interface_) {
        ImGui::Text("Queued buffers: %d", audio_interface_->GetQueuedBufferCount());
    }

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

bool PlayerNode::AddInput(AudioNode* input_node, int /*pin_id*/) {
    if (!input_node) return false;
    
    if (!input_) {
        input_ = input_node;
        return true;
    }
    
    return false;
}

void PlayerNode::RemoveInput(AudioNode* input_node, int /*pin_id*/) {
    if (input_ == input_node) {
        input_ = nullptr;
    }
}

void PlayerNode::OnGraphChanged() {
    needs_stream_prime_ = true;
}

void PlayerNode::SetPlaying(bool playing) {
    if (playing_ == playing) {
        return;
    }

    playing_ = playing;

    if (!audio_interface_) {
        return;
    }

    if (!playing_) {
        audio_interface_->Stop();
        needs_stream_prime_ = true;
        LOG(INFO) << "Player node stopped";
        return;
    }

    audio_interface_->Stop();
    audio_interface_->SetVolume(volume_);
    needs_stream_prime_ = true;
    LOG(INFO) << "Player node started";
}

void PlayerNode::UpdateAudio(int sample_rate) {
    if (!audio_interface_) {
        return;
    }

    audio_interface_->ServiceStreamingQueue();

    if (!playing_) {
        return;
    }

    UpdateStreaming(sample_rate);
}

void PlayerNode::UpdateStreaming(int sample_rate) {
    if (!audio_interface_) {
        return;
    }

    sample_rate_ = sample_rate;

    if (needs_stream_prime_) {
        audio_interface_->ClearStreamingQueue();
        needs_stream_prime_ = false;
    }

    audio_interface_->SetVolume(volume_);

    while (audio_interface_->GetQueuedBufferCount() < max_queue_buffers_) {
        if (!QueueGeneratedAudio(sample_rate)) {
            break;
        }
    }
}

bool PlayerNode::QueueGeneratedAudio(int sample_rate) {
    const int buffer_size = streaming_buffer_size_ > 0 ? streaming_buffer_size_ : 1024;
    auto audio_data = GenerateAudio(buffer_size, sample_rate);
    if (audio_data.empty()) {
        return false;
    }

    for (auto& sample : audio_data) {
        sample *= volume_;
        sample = std::max(-1.0f, std::min(1.0f, sample));
    }

    AppendToHistory(audio_data);

    pcm_convert_buffer_.resize(audio_data.size());
    for (size_t i = 0; i < audio_data.size(); ++i) {
        pcm_convert_buffer_[i] = static_cast<short>(audio_data[i] * 32767.0f);
    }

    return audio_interface_->AppendSamples(pcm_convert_buffer_, sample_rate, false);
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

    AppendCaptureSamples(samples);
    
    UpdateSpectrogram(samples);
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

    ImGui::InputInt("Capture Samples", &capture_target_input_);
    if (capture_target_input_ < 1) {
        capture_target_input_ = 1;
    }

    if (ImGui::Button("Start Capture")) {
        StartCapture();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Capture")) {
        capture_buffer_.clear();
        capture_samples_collected_ = 0;
        capture_active_ = false;
        capture_ready_ = false;
    }

    if (capture_active_) {
        ImGui::Text("Capturing... %zu / %zu samples",
                    capture_samples_collected_, capture_target_samples_);
    } else if (capture_ready_) {
        ImGui::Text("Capture complete: %zu samples", capture_buffer_.size());
    } else {
        ImGui::Text("Capture idle");
    }

    if (capture_buffer_.empty()) {
        ImGui::Text("No captured samples yet.");
        ImGui::End();
        return;
    }

    int sample_count = 0;
    const float* plot_data = PreparePlotData(capture_buffer_, &sample_count);
    if (!plot_data || sample_count <= 0) {
        ImGui::Text("No data available for plotting.");
        ImGui::End();
        return;
    }

    ImGui::Text("Plotting %d points (downsampled from %zu)", sample_count,
                capture_buffer_.size());

    if (ImPlot::BeginPlot("Captured Buffers", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Sample", "Amplitude",
                          ImPlotAxisFlags_NoGridLines,
                          ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxesLimits(0.0,
                                static_cast<double>(sample_count),
                                -1.1,
                                1.1,
                                ImPlotCond_Once);
        ImPlot::PlotLine("History",
                         plot_data,
                         sample_count);
        ImPlot::EndPlot();
    }

    ImGui::End();
}

const float* PlayerNode::PreparePlotData(const std::vector<float>& samples, int* sample_count) {
    if (sample_count == nullptr) {
        return nullptr;
    }

    if (samples.empty()) {
        *sample_count = 0;
        return nullptr;
    }

    if (samples.size() <= max_plot_samples_) {
        *sample_count = static_cast<int>(samples.size());
        return samples.data();
    }

    const size_t stride =
        (samples.size() + max_plot_samples_ - 1) / max_plot_samples_;
    const size_t downsampled_count =
        (samples.size() + stride - 1) / stride;
    plot_scratch_buffer_.resize(downsampled_count);

    size_t idx = 0;
    for (size_t i = 0; i < samples.size(); i += stride) {
        plot_scratch_buffer_[idx++] = samples[i];
    }

    *sample_count = static_cast<int>(idx);
    return plot_scratch_buffer_.data();
}

void PlayerNode::StartCapture() {
    capture_target_samples_ = static_cast<size_t>(std::max(1, capture_target_input_));
    capture_buffer_.clear();
    capture_buffer_.reserve(capture_target_samples_);
    capture_samples_collected_ = 0;
    capture_active_ = true;
    capture_ready_ = false;
}

void PlayerNode::AppendCaptureSamples(const std::vector<float>& samples) {
    if (!capture_active_ || samples.empty()) {
        return;
    }

    size_t remaining = capture_target_samples_ - capture_samples_collected_;
    if (remaining == 0) {
        capture_active_ = false;
        capture_ready_ = true;
        return;
    }

    size_t to_copy = std::min(remaining, samples.size());
    capture_buffer_.insert(capture_buffer_.end(), samples.begin(), samples.begin() + to_copy);
    capture_samples_collected_ += to_copy;

    if (capture_samples_collected_ >= capture_target_samples_) {
        capture_active_ = false;
        capture_ready_ = true;
    }
}

void PlayerNode::ComputeFFT(const float* input, int size, std::vector<float>& magnitudes) {
    // Use PocketFFT for efficient FFT computation
    // Only compute first half of spectrum (positive frequencies)
    int half_size = size / 2;
    magnitudes.resize(half_size);
    
    // Prepare input for PocketFFT (copy to complex vector)
    std::vector<std::complex<float>> fft_data(size);
    for (int i = 0; i < size; ++i) {
        fft_data[i] = std::complex<float>(input[i], 0.0f);
    }
    
    // Perform FFT using PocketFFT
    pocketfft::shape_t shape{static_cast<size_t>(size)};
    pocketfft::stride_t stride_in{sizeof(std::complex<float>)};
    pocketfft::stride_t stride_out{sizeof(std::complex<float>)};
    pocketfft::shape_t axes{0};
    
    pocketfft::c2c(shape, stride_in, stride_out, axes, 
                   pocketfft::FORWARD,
                   fft_data.data(), fft_data.data(), 1.0f);
    
    // Compute magnitudes and convert to dB scale
    for (int k = 0; k < half_size; ++k) {
        float real_part = fft_data[k].real();
        float imag_part = fft_data[k].imag();
        
        // Compute magnitude
        float magnitude = std::sqrt(real_part * real_part + imag_part * imag_part);
        magnitude = magnitude / size; // Normalize
        
        // Convert to dB (with floor to avoid log(0))
        float db = 20.0f * std::log10(std::max(magnitude, 1e-6f));
        magnitudes[k] = db;
    }
}

void PlayerNode::UpdateSpectrogram(const std::vector<float>& samples) {
    // Accumulate samples into FFT input buffer
    for (float sample : samples) {
        fft_input_buffer_.erase(fft_input_buffer_.begin());
        fft_input_buffer_.push_back(sample);
    }
    
    spectrogram_sample_counter_ += static_cast<int>(samples.size());
    
    // Only compute FFT when we've accumulated enough samples (hop size = FFT size / 4)
    // This prevents excessive FFT computation with small buffer sizes
    int hop_size = fft_size_ / 4;
    if (spectrogram_sample_counter_ < hop_size) {
        return;
    }
    
    spectrogram_sample_counter_ = 0;
    
    // Apply window and compute FFT
    std::vector<float> windowed(fft_size_);
    for (int i = 0; i < fft_size_; ++i) {
        windowed[i] = fft_input_buffer_[i] * fft_window_[i];
    }
    
    std::vector<float> magnitudes;
    ComputeFFT(windowed.data(), fft_size_, magnitudes);
    
    // Add to spectrogram data (shift old data)
    spectrogram_data_.push_back(magnitudes);
    
    // Keep only recent time slices
    if (spectrogram_data_.size() > spectrogram_time_slices_) {
        spectrogram_data_.pop_front();
    }
}



void PlayerNode::DrawSpectrogramContent() {
    ImGui::Text("Live Spectrogram View");
    if (ImGui::SliderInt("FFT Size", &fft_size_, 128, 16384)) {
        // Automatically reset buffers when FFT size changes
        fft_input_buffer_.resize(fft_size_, 0.0f);
        fft_window_.resize(fft_size_);
        for (int i = 0; i < fft_size_; ++i) {
            fft_window_[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265359f * i / (fft_size_ - 1)));
        }
        spectrogram_data_.clear();
        spectrogram_sample_counter_ = 0;
    }
    
    int time_slices_int = static_cast<int>(spectrogram_time_slices_);
    if (ImGui::SliderInt("Time Slices", &time_slices_int, 10, 500)) {
        spectrogram_time_slices_ = static_cast<size_t>(time_slices_int);
    }
    
    if (ImGui::Button("Reset FFT Size")) {
        fft_input_buffer_.resize(fft_size_, 0.0f);
        fft_window_.resize(fft_size_);
        for (int i = 0; i < fft_size_; ++i) {
            fft_window_[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265359f * i / (fft_size_ - 1)));
        }
        spectrogram_data_.clear();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        spectrogram_data_.clear();
    }
    
    if (spectrogram_data_.empty()) {
        ImGui::Text("No spectrogram data yet. Start playing audio.");
        return;
    }
    
    // Flatten spectrogram data for PlotHeatmap
    // PlotHeatmap expects data[row][col] in row-major order
    // Our data: spectrogram_data_[time_slice][freq_bin]
    // We need to transpose: rows = frequency bins, cols = time slices
    int rows = static_cast<int>(spectrogram_data_[0].size()); // frequency bins
    int cols = static_cast<int>(spectrogram_data_.size());     // time slices
    std::vector<float> heatmap_data(rows * cols);
    
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            heatmap_data[row * cols + col] = spectrogram_data_[col][row];
        }
    }
    
    float max_frequency = static_cast<float>(sample_rate_) / 2.0f;
    
    // Initialize frequency axis limits if needed
    if (frequency_axis_min_ == 0.0 && frequency_axis_max_ == 24000.0) {
        frequency_axis_min_ = 0.0;
        frequency_axis_max_ = static_cast<double>(max_frequency);
    }
    
    // Use subplots: Spectrogram on left, PSD on right
    if (ImPlot::BeginSubplots("Spectrum Analysis", 1, 2, ImVec2(-1, -1))) {
        // Left subplot: Spectrogram (Time vs Frequency)
        // Link Y-axis (frequency) to shared variables BEFORE BeginPlot
        ImPlot::SetNextAxisLinks(ImAxis_Y1, &frequency_axis_min_, &frequency_axis_max_);
        
        if (ImPlot::BeginPlot("Spectrogram")) {
            ImPlot::SetupAxes("Time Slice", "Frequency (Hz)");
            
            // Set up bounds for the heatmap in plot coordinates
            // Flip Y-axis: row 0 (lowest freq) should be at bottom, so bounds_min.y > bounds_max.y
            ImPlotPoint bounds_min(0, max_frequency);
            ImPlotPoint bounds_max(cols, 0);
            
            ImPlot::SetupAxesLimits(0, static_cast<double>(cols),
                                   0, static_cast<double>(max_frequency),
                                   ImPlotCond_Once);
            
            // Use a colormap suitable for spectrograms (Viridis, Hot, or Plasma work well)
            ImPlot::PushColormap(ImPlotColormap_Hot);
            
            // Plot heatmap with dB range as scale
            ImPlot::PlotHeatmap("##heatmap", heatmap_data.data(), rows, cols, 
                               -80.0, 0.0, nullptr, bounds_min, bounds_max);
            
            ImPlot::PopColormap();
            
            ImPlot::EndPlot();
        }
        
        // Right subplot: Power Spectral Density (Frequency vs Magnitude)
        // Link X-axis (frequency) to shared variables BEFORE BeginPlot
        ImPlot::SetNextAxisLinks(ImAxis_X1, &frequency_axis_min_, &frequency_axis_max_);
        
        if (ImPlot::BeginPlot("Power Spectral Density")) {
            ImPlot::SetupAxes("Frequency (Hz)", "Magnitude (dB)");
            
            ImPlot::SetupAxesLimits(0, static_cast<double>(max_frequency),
                                   -80, 0,
                                   ImPlotCond_Once);
            
            // Use the most recent spectrum data
            if (!spectrogram_data_.empty()) {
                const auto& latest_spectrum = spectrogram_data_.back();
                int num_bins = static_cast<int>(latest_spectrum.size());
                
                // Create frequency axis data
                std::vector<double> frequencies(num_bins);
                for (int i = 0; i < num_bins; ++i) {
                    frequencies[i] = (static_cast<double>(i) / num_bins) * max_frequency;
                }
                
                // Convert to double for plotting
                std::vector<double> magnitudes(num_bins);
                for (int i = 0; i < num_bins; ++i) {
                    magnitudes[i] = static_cast<double>(latest_spectrum[i]);
                }
                
                ImPlot::PlotLine("PSD", frequencies.data(), magnitudes.data(), num_bins);
            }
            
            ImPlot::EndPlot();
        }
        
        ImPlot::EndSubplots();
    }
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
