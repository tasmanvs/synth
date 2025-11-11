#include "imgui.h"
#include "implot.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ui/main_window.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>

// Emscripten requires a global loop function
GLFWwindow* g_window = nullptr;
MainWindow* g_main_window = nullptr;

void MainLoop()
{
    // Poll and handle events
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Update and draw main window
    g_main_window->Update();
    g_main_window->Draw();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_window);
}

int main(int argc, char** argv)
{
    // Initialize Abseil logging
    absl::InitializeLog();
    
    // Set stderr threshold to INFO so logs appear in browser console
    absl::SetStderrThreshold(absl::LogSeverity::kInfo);
    
    LOG(INFO) << "Starting ImGui WebGL application";
    
    // Setup GLFW
    if (!glfwInit())
    {
        LOG(ERROR) << "Failed to initialize GLFW";
        printf("Failed to initialize GLFW\n");
        return -1;
    }
    LOG(INFO) << "GLFW initialized successfully";

    // For the browser using Emscripten, we are going to use WebGL2 with GL ES3
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Create window with graphics context
    g_window = glfwCreateWindow(1280, 720, "Dear ImGui - Bazel + Emscripten + WebGL", nullptr, nullptr);
    if (g_window == nullptr)
    {
        LOG(ERROR) << "Failed to create GLFW window";
        printf("Failed to create GLFW window\n");
        return -1;
    }
    LOG(INFO) << "GLFW window created (1280x720)";
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    LOG(INFO) << "ImGui platform/renderer backends initialized";
    
    // Install Emscripten-specific callbacks (including wheel callback for proper scrolling)
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(g_window, "#canvas");
    LOG(INFO) << "Emscripten callbacks installed";

    // Create main window
    g_main_window = new MainWindow();
    LOG(INFO) << "MainWindow created, starting main loop";

    // This function call won't return, and will engage in an infinite loop
    emscripten_set_main_loop(MainLoop, 0, true);

    // Cleanup (this will never be reached in Emscripten)
    delete g_main_window;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(g_window);
    glfwTerminate();

    return 0;
}
