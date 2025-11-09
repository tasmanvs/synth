#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

// Data
static ID3D11Device*            g_pd3d_device = nullptr;
static ID3D11DeviceContext*     g_pd3d_device_context = nullptr;
static IDXGISwapChain*          g_p_swap_chain = nullptr;
static bool                     g_swap_chain_occluded = false;
static UINT                     g_resize_width = 0, g_resize_height = 0;
static ID3D11RenderTargetView*  g_main_render_target_view = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND h_wnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

// Main code
int main(int, char**)
{
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND h_wnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui - Bazel + DirectX11 Hello World", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(h_wnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(h_wnd, SW_SHOWDEFAULT);
    ::UpdateWindow(h_wnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(h_wnd);
    ImGui_ImplDX11_Init(g_pd3d_device, g_pd3d_device_context);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window screen locked
        if (g_swap_chain_occluded && g_p_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_swap_chain_occluded = false;

        // Handle window resize
        if (g_resize_width != 0 && g_resize_height != 0)
        {
            CleanupRenderTarget();
            g_p_swap_chain->ResizeBuffers(0, g_resize_width, g_resize_height, DXGI_FORMAT_UNKNOWN, 0);
            g_resize_width = g_resize_height = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, Bazel + ImGui + DirectX11!");

            ImGui::Text("This is ImGui running on Windows with DirectX11!");
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);

            if (ImGui::Button("Button"))
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                       1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3d_device_context->OMSetRenderTargets(1, &g_main_render_target_view, nullptr);
        g_pd3d_device_context->ClearRenderTargetView(g_main_render_target_view, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_p_swap_chain->Present(1, 0);
        g_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(h_wnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND h_wnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = h_wnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT create_device_flags = 0;
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_level_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_level_array, 2, D3D11_SDK_VERSION, &sd, &g_p_swap_chain, &g_pd3d_device, &feature_level, &g_pd3d_device_context);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_p_swap_chain) { g_p_swap_chain->Release(); g_p_swap_chain = nullptr; }
    if (g_pd3d_device_context) { g_pd3d_device_context->Release(); g_pd3d_device_context = nullptr; }
    if (g_pd3d_device) { g_pd3d_device->Release(); g_pd3d_device = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* p_back_buffer;
    g_p_swap_chain->GetBuffer(0, IID_PPV_ARGS(&p_back_buffer));
    g_pd3d_device->CreateRenderTargetView(p_back_buffer, nullptr, &g_main_render_target_view);
    p_back_buffer->Release();
}

void CleanupRenderTarget()
{
    if (g_main_render_target_view) { g_main_render_target_view->Release(); g_main_render_target_view = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(h_wnd, msg, w_param, l_param))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (w_param == SIZE_MINIMIZED)
            return 0;
        g_resize_width = (UINT)LOWORD(l_param);
        g_resize_height = (UINT)HIWORD(l_param);
        return 0;
    case WM_SYSCOMMAND:
        if ((w_param & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(h_wnd, msg, w_param, l_param);
}
