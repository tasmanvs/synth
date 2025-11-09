#include "ui/main_window.h"
#include <cmath>
#include "absl/log/log.h"
#include "absl/log/check.h"

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
    , sample_rate_(44100)
{
    LOG(INFO) << "MainWindow initialized with sample rate: " << sample_rate_;
}

MainWindow::~MainWindow()
{
}


void MainWindow::Update()
{
    // Update audio playback
    static bool was_playing = false;
    static float last_frequency = 0.0f;
    static float last_volume = 0.0f;
    
    if (playing_ != was_playing)
    {
        if (playing_)
        {
            LOG(INFO) << "Starting audio tone at " << frequency_ << " Hz, volume " << volume_;
            
            // Generate waveform using synth
            auto samples = audio_synth_.generateSineWaveCycle(frequency_, sample_rate_, volume_);
            
            // Play using audio interface
            audio_interface_.playSamples(samples, sample_rate_, true);
            audio_interface_.play();
        }
        else
        {
            LOG(INFO) << "Stopping audio tone";
            audio_interface_.stop();
        }
        was_playing = playing_;
        last_frequency = frequency_;
        last_volume = volume_;
    }
    else if (playing_ && (frequency_ != last_frequency || volume_ != last_volume))
    {
        LOG(INFO) << "Updating tone: frequency=" << frequency_ << " Hz, volume=" << volume_;
        
        // Stop current playback
        audio_interface_.stop();
        
        // Generate new waveform
        auto samples = audio_synth_.generateSineWaveCycle(frequency_, sample_rate_, volume_);
        
        // Play new waveform
        audio_interface_.playSamples(samples, sample_rate_, true);
        audio_interface_.play();
        
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
        
        // Show waveform from audio interface
        if (playing_)
        {
            const auto& samples = audio_interface_.getCurrentSamples();
            if (!samples.empty())
            {
                // Convert short samples to float for visualization
                std::vector<float> float_samples(samples.size());
                for (size_t i = 0; i < samples.size(); ++i)
                {
                    float_samples[i] = samples[i] / 32767.0f;
                }
                
                ImGui::PlotLines("Waveform", float_samples.data(), 
                                static_cast<int>(float_samples.size()), 0, nullptr, -1.0f, 1.0f, 
                                ImVec2(0, 80));
            }
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
