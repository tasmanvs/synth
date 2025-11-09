#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "main_window.h"
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>

// Emscripten requires a global loop function
GLFWwindow* g_Window = nullptr;
MainWindow* g_MainWindow = nullptr;

void main_loop()
{
    // Poll and handle events
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Update and draw main window
    g_MainWindow->Update();
    g_MainWindow->Draw();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_Window);
}

int main(int, char**)
{
    // Setup GLFW
    if (!glfwInit())
    {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    // For the browser using Emscripten, we are going to use WebGL2 with GL ES3
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Create window with graphics context
    g_Window = glfwCreateWindow(1280, 720, "Dear ImGui - Bazel + Emscripten + WebGL", nullptr, nullptr);
    if (g_Window == nullptr)
    {
        printf("Failed to create GLFW window\n");
        return -1;
    }
    glfwMakeContextCurrent(g_Window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Create main window
    g_MainWindow = new MainWindow();

    // This function call won't return, and will engage in an infinite loop
    emscripten_set_main_loop(main_loop, 0, true);

    // Cleanup (this will never be reached in Emscripten)
    delete g_MainWindow;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(g_Window);
    glfwTerminate();

    return 0;
}
