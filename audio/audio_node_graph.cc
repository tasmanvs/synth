#include "audio/audio_node_graph.h"
#include "absl/log/log.h"
#include "pocketfft_hdronly.h"
#include <algorithm>
#include <complex>

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
    , show_spectrogram_(false)
    , spectrogram_time_slices_(100)
    , fft_size_(512)
    , fft_input_buffer_(fft_size_, 0.0f)
    , sample_rate_(48000)
    , spectrogram_sample_counter_(0) {
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
    
    ImGui::SameLine();
    if (ImGui::Button("Spectrogram")) {
        show_spectrogram_ = true;
    }
    
    ed::EndNode();
    ImGui::PopID();

    if (show_debug_window_) {
        ed::Suspend();
        DrawHistoryWindow();
        ed::Resume();
    }
    
    if (show_spectrogram_) {
        ed::Suspend();
        DrawSpectrogramView();
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
    
    if (show_spectrogram_) {
        UpdateSpectrogram(samples);
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

void PlayerNode::DrawSpectrogramView() {
    if (!show_spectrogram_) {
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Spectrogram", &show_spectrogram_)) {
        ImGui::End();
        return;
    }
    
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
        ImGui::End();
        return;
    }
    
    // Draw spectrogram as a heatmap
    if (ImPlot::BeginPlot("Spectrogram", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time Slice", "Frequency (Hz)");
        float max_frequency = static_cast<float>(sample_rate_) / 2.0f;
        ImPlot::SetupAxesLimits(0, static_cast<double>(spectrogram_data_.size()),
                               0, static_cast<double>(max_frequency),
                               ImPlotCond_Once);
        
        // Draw as a heatmap using lines
        for (size_t time_idx = 0; time_idx < spectrogram_data_.size(); ++time_idx) {
            const auto& slice = spectrogram_data_[time_idx];
            
            // Normalize and colorize based on magnitude
            float freq_bin_to_hz = static_cast<float>(sample_rate_) / static_cast<float>(fft_size_);
            
            for (size_t freq_idx = 0; freq_idx < slice.size(); ++freq_idx) {
                float db = slice[freq_idx];
                
                // Map dB to color intensity (-80 dB to 0 dB range)
                float normalized = (db + 80.0f) / 80.0f;
                normalized = std::max(0.0f, std::min(1.0f, normalized));
                
                if (normalized > 0.1f) { // Only draw if above threshold
                    // Color from blue (low) to red (high)
                    ImVec4 color;
                    if (normalized < 0.5f) {
                        color = ImVec4(0.0f, normalized * 2.0f, 1.0f - normalized * 2.0f, normalized);
                    } else {
                        color = ImVec4((normalized - 0.5f) * 2.0f, 1.0f - (normalized - 0.5f) * 2.0f, 0.0f, normalized);
                    }
                    
                    ImPlot::SetNextLineStyle(color, 2.0f);
                    
                    // Convert bin indices to frequency in Hz
                    double x[2] = {static_cast<double>(time_idx), static_cast<double>(time_idx)};
                    double y[2] = {freq_idx * freq_bin_to_hz, (freq_idx + 1) * freq_bin_to_hz};
                    ImPlot::PlotLine("##spec", x, y, 2);
                }
            }
        }
        
        ImPlot::EndPlot();
    }
    
    ImGui::End();
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
