#include "audio/nodes/player_node.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include "implot.h"
#include "absl/log/log.h"
#include "pocketfft_hdronly.h"
#include <cmath>
#include <algorithm>
#include <complex>

namespace audio_nodes {

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

    if (!ImGui::Begin("Player Visualization", &show_debug_window_)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("VizTabs")) {
        if (ImGui::BeginTabItem("Waveform History")) {
            if (playback_history_.empty()) {
                ImGui::Text("No buffers captured yet.");
            } else {
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

                if (!capture_buffer_.empty()) {
                    int sample_count = 0;
                    const float* plot_data = PreparePlotData(capture_buffer_, &sample_count);
                    if (plot_data && sample_count > 0) {
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
                    } else {
                        ImGui::Text("No data available for plotting.");
                    }
                } else {
                    ImGui::Text("No captured samples yet.");
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Spectrogram")) {
            DrawSpectrogramContent();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
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

} // namespace audio_nodes
