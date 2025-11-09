#include "main_window.h"

MainWindow::MainWindow()
    : show_demo_window_(true)
    , show_implot_demo_window_(true)
    , show_another_window_(false)
    , clear_color_(0.45f, 0.55f, 0.60f, 1.00f)
    , slider_value_(0.0f)
    , counter_(0)
{
}

MainWindow::~MainWindow()
{
}

void MainWindow::Update()
{
    // Any per-frame update logic can go here
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
