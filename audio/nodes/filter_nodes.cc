#include "audio/nodes/filter_nodes.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include "implot.h"
#include <cmath>
#include <complex>
#include <algorithm>

namespace audio_nodes {

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

} // namespace audio_nodes
