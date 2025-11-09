#include "main_window.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
{
    audio_buffer_.resize(1024);
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

void MainWindow::Update()
{
    GenerateAudioSamples();
    
    // Update OpenAL audio
    static bool was_playing = false;
    static float last_frequency = 0.0f;
    static float last_volume = 0.0f;
    
    if (playing_ != was_playing)
    {
        if (playing_)
        {
            audio_synth_.startTone(frequency_, volume_);
        }
        else
        {
            audio_synth_.stopTone();
        }
        was_playing = playing_;
        last_frequency = frequency_;
        last_volume = volume_;
    }
    else if (playing_ && (frequency_ != last_frequency || volume_ != last_volume))
    {
        audio_synth_.updateTone(frequency_, volume_);
        last_frequency = frequency_;
        last_volume = volume_;
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
        
        if (ImGui::Button(playing_ ? "Stop" : "Play"))
            playing_ = !playing_;
        
        ImGui::SliderFloat("Frequency (Hz)", &frequency_, 20.0f, 2000.0f, "%.1f Hz");
        ImGui::SliderFloat("Volume", &volume_, 0.0f, 1.0f);
        
        // Show waveform
        if (playing_ && !audio_buffer_.empty())
        {
            ImGui::PlotLines("Waveform", audio_buffer_.data(), 
                            static_cast<int>(audio_buffer_.size()), 0, nullptr, -1.0f, 1.0f, 
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
