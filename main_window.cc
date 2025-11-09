#include "main_window.h"
#include <cmath>
#include <emscripten.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Declare JavaScript functions
extern "C" {
    void js_init_audio();
    void js_start_tone(float frequency, float volume);
    void js_stop_tone();
    void js_update_tone(float frequency, float volume);
    void js_play_buffer(float* buffer, int length, int sample_rate, float volume);
    void js_stop_buffer();
    void js_update_buffer_volume(float volume);
}

MainWindow::MainWindow()
    : show_demo_window_(true)
    , show_implot_demo_window_(true)
    , show_another_window_(false)
    , clear_color_(0.45f, 0.55f, 0.60f, 1.00f)
    , slider_value_(0.0f)
    , counter_(0)
    , frequency_(440.0f)
    , volume_(0.3f)
    , playing_(false)
    , phase_(0.0)
    , sample_rate_(44100)
    , playing_custom_(false)
    , regenerate_requested_(false)
    , custom_buffer_size_(44100)  // 1 second of audio
{
    audio_buffer_.resize(1024);
    custom_buffer_.resize(custom_buffer_size_);
    GenerateCustomBuffer();
}

MainWindow::~MainWindow()
{
}

void MainWindow::GenerateAudioSamples()
{
    if (!playing_) return;
    
    for (size_t i = 0; i < audio_buffer_.size(); ++i)
    {
        audio_buffer_[i] = volume_ * std::sin(2.0 * M_PI * frequency_ * phase_);
        phase_ += 1.0 / sample_rate_;
        if (phase_ >= 1.0)
            phase_ -= 1.0;
    }
}

void MainWindow::GenerateCustomBuffer()
{
    // Generate a custom waveform - for now, a richer sound with multiple harmonics
    for (int i = 0; i < custom_buffer_size_; ++i)
    {
        float t = static_cast<float>(i) / sample_rate_;
        float freq = 220.0f;  // A3 note
        
        // Create a complex waveform with multiple harmonics
        float sample = 0.0f;
        sample += 0.5f * std::sin(2.0f * M_PI * freq * t);           // Fundamental
        sample += 0.25f * std::sin(2.0f * M_PI * freq * 2.0f * t);   // 2nd harmonic
        sample += 0.125f * std::sin(2.0f * M_PI * freq * 3.0f * t);  // 3rd harmonic
        sample += 0.0625f * std::sin(2.0f * M_PI * freq * 4.0f * t); // 4th harmonic
        
        // Apply envelope (fade in and fade out)
        float envelope = 1.0f;
        float attack_time = 0.1f;
        float release_time = 0.2f;
        
        if (t < attack_time) {
            envelope = t / attack_time;
        } else if (t > (custom_buffer_size_ / (float)sample_rate_) - release_time) {
            float time_from_end = (custom_buffer_size_ / (float)sample_rate_) - t;
            envelope = time_from_end / release_time;
        }
        
        custom_buffer_[i] = sample * envelope * 0.3f;
    }
}

void MainWindow::Update()
{
    GenerateAudioSamples();
    
    // Update Web Audio API for oscillator
    static bool was_playing = false;
    static float last_frequency = 0.0f;
    static float last_volume = 0.0f;
    
    if (playing_ != was_playing)
    {
        if (playing_)
        {
            js_start_tone(frequency_, volume_);
        }
        else
        {
            js_stop_tone();
        }
        was_playing = playing_;
        last_frequency = frequency_;
        last_volume = volume_;
    }
    else if (playing_ && (frequency_ != last_frequency || volume_ != last_volume))
    {
        js_update_tone(frequency_, volume_);
        last_frequency = frequency_;
        last_volume = volume_;
    }
    
    // Update Web Audio API for custom buffer
    static bool was_playing_custom = false;
    static float last_custom_volume = 0.0f;
    
    // Handle regeneration request
    if (regenerate_requested_)
    {
        if (playing_custom_)
        {
            // Stop and restart with new buffer
            js_stop_buffer();
            js_play_buffer(custom_buffer_.data(), custom_buffer_size_, sample_rate_, volume_);
        }
        regenerate_requested_ = false;
    }
    
    if (playing_custom_ != was_playing_custom)
    {
        if (playing_custom_)
        {
            js_play_buffer(custom_buffer_.data(), custom_buffer_size_, sample_rate_, volume_);
        }
        else
        {
            js_stop_buffer();
        }
        was_playing_custom = playing_custom_;
        last_custom_volume = volume_;
    }
    else if (playing_custom_ && volume_ != last_custom_volume)
    {
        js_update_buffer_volume(volume_);
        last_custom_volume = volume_;
    }
}

void MainWindow::Draw()
{
    // 1. Show the big demo window
    if (show_demo_window_)
        ImGui::ShowDemoWindow(&show_demo_window_);

    // 2. Show the ImPlot demo window
    if (show_implot_demo_window_)
        ImPlot::ShowDemoWindow(&show_implot_demo_window_);

    // 3. Show a simple window
    {
        ImGui::Begin("Hello, Bazel + ImGui + WebGL!");

        ImGui::Text("This is ImGui running in a web browser with WebGL!");
        ImGui::Text("Built with Bazel and Emscripten!");
        ImGui::Checkbox("ImGui Demo Window", &show_demo_window_);
        ImGui::Checkbox("ImPlot Demo Window", &show_implot_demo_window_);
        ImGui::Checkbox("Another Window", &show_another_window_);

        ImGui::Separator();
        ImGui::Text("Audio Synthesizer");
        
        if (ImGui::Button(playing_ ? "Stop Oscillator" : "Play Oscillator"))
            playing_ = !playing_;
        
        ImGui::SameLine();
        if (ImGui::Button(playing_custom_ ? "Stop Custom Buffer" : "Play Custom Buffer"))
            playing_custom_ = !playing_custom_;
        
        ImGui::SliderFloat("Frequency (Hz)", &frequency_, 20.0f, 2000.0f, "%.1f Hz");
        ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f);
        
        if (ImGui::Button("Regenerate Custom Buffer"))
        {
            GenerateCustomBuffer();
            regenerate_requested_ = true;
        }
        
        // Show waveform
        if (playing_ && !audio_buffer_.empty())
        {
            ImGui::PlotLines("Oscillator Waveform", audio_buffer_.data(), 
                            static_cast<int>(audio_buffer_.size()), 0, nullptr, -1.0f, 1.0f, 
                            ImVec2(0, 80));
        }
        
        // Show custom buffer waveform (first 1024 samples)
        if (!custom_buffer_.empty())
        {
            int plot_samples = std::min(1024, custom_buffer_size_);
            ImGui::PlotLines("Custom Buffer", custom_buffer_.data(), 
                            plot_samples, 0, nullptr, -1.0f, 1.0f, 
                            ImVec2(0, 80));
        }

        ImGui::Separator();
        ImGui::SliderFloat("float", &slider_value_, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color_);

        if (ImGui::Button("Button"))
            counter_++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter_);

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                   1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 4. Show another simple window
    if (show_another_window_)
    {
        ImGui::Begin("Another Window", &show_another_window_);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window_ = false;
        ImGui::End();
    }
}
