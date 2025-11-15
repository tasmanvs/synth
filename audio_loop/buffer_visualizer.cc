#include "imgui.h"
#include "implot.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include <d3d11.h>
#include <tchar.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "audio_loop/audio_output.h"
#include "audio_loop/sine_buffer_generator.h"

namespace {

constexpr float kTwoPi = 6.28318530717958647692f;
constexpr int kMinBufferCount = 2;
constexpr int kMaxBufferCount = 6;
constexpr int kMinFrameCount = 32;
constexpr int kMaxFrameCount = 4096;
constexpr int kMinSampleRate = 8000;
constexpr int kMaxSampleRate = 96000;

struct VisualizerState {
    audio_loop::BufferConfig config;
    int buffer_count = 3;
    float start_phase = 0.0f;
    bool auto_refresh = true;
    bool show_combined = false;

    audio_loop::PhaseContinuousSine generator;
    std::vector<std::vector<float>> buffers;
    std::vector<audio_loop::ContinuityResult> continuity_results;
    std::vector<float> concatenated;

    audio_loop::AudioBufferPlayer audio_player;
    float playback_volume = 0.6f;
    bool audio_ready = false;
    bool audio_failed = false;
    std::string audio_message;
};

void RegenerateBuffers(VisualizerState* state) {
    if (state->buffer_count < kMinBufferCount) {
        state->buffer_count = kMinBufferCount;
    }
    if (state->buffer_count > kMaxBufferCount) {
        state->buffer_count = kMaxBufferCount;
    }

    audio_loop::PhaseContinuousSine generator(state->start_phase);
    state->buffers.clear();
    state->continuity_results.clear();
    state->buffers.reserve(static_cast<size_t>(state->buffer_count));
    if (state->buffer_count > 1) {
        state->continuity_results.resize(static_cast<size_t>(state->buffer_count - 1));
    }

    for (int i = 0; i < state->buffer_count; ++i) {
        state->buffers.push_back(generator.GenerateBuffer(state->config));
        if (i > 0) {
            state->continuity_results[static_cast<size_t>(i - 1)] =
                audio_loop::EvaluateContinuity(state->buffers[static_cast<size_t>(i - 1)],
                                               state->buffers[static_cast<size_t>(i)], 0.2f);
        }
    }

    state->generator = generator;
    state->concatenated = audio_loop::ConcatenateBuffers(state->buffers);
}

bool EnsureAudioInitialized(VisualizerState* state) {
    if (state->audio_ready) {
        return true;
    }
    if (state->audio_player.Initialize()) {
        state->audio_ready = true;
        state->audio_failed = false;
        state->audio_player.SetVolume(state->playback_volume);
        state->audio_message = "Audio engine ready.";
        return true;
    }
    state->audio_failed = true;
    state->audio_ready = false;
    state->audio_message = "Failed to initialize XAudio2. Install the latest DirectX runtime.";
    return false;
}

void DrawPlaybackSection(VisualizerState* state) {
    ImGui::Separator();
    ImGui::Text("Audio Playback");

    if (!state->audio_ready) {
        if (!state->audio_message.empty()) {
            ImGui::TextWrapped("%s", state->audio_message.c_str());
        }
        if (ImGui::Button("Retry Audio Init")) {
            EnsureAudioInitialized(state);
        }
        return;
    }

    if (ImGui::SliderFloat("Playback Volume", &state->playback_volume, 0.0f, 1.0f)) {
        state->audio_player.SetVolume(state->playback_volume);
    }

    const bool has_samples = !state->concatenated.empty();
    ImGui::BeginDisabled(!has_samples);
    if (ImGui::Button("Play Buffers")) {
        if (state->audio_player.Play(state->concatenated, state->config.sample_rate)) {
            state->audio_message = "Playing concatenated buffer.";
        } else {
            state->audio_message = "Failed to start playback.";
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Stop Playback")) {
        state->audio_player.Stop();
        state->audio_message = "Playback stopped.";
    }

    ImGui::Text("Status: %s", state->audio_player.IsPlaying() ? "Playing" : "Idle");
    if (!state->audio_message.empty()) {
        ImGui::TextWrapped("%s", state->audio_message.c_str());
    }
}

// DirectX state copied from //examples:imgui_dx11
static ID3D11Device* g_pd3d_device = nullptr;
static ID3D11DeviceContext* g_pd3d_device_context = nullptr;
static IDXGISwapChain* g_p_swap_chain = nullptr;
static bool g_swap_chain_occluded = false;
static UINT g_resize_width = 0, g_resize_height = 0;
static ID3D11RenderTargetView* g_main_render_target_view = nullptr;

const std::array<ImVec4, 6> kBufferColors = {
    ImVec4(0.94f, 0.33f, 0.31f, 1.0f),
    ImVec4(0.18f, 0.52f, 0.88f, 1.0f),
    ImVec4(0.46f, 0.82f, 0.34f, 1.0f),
    ImVec4(0.89f, 0.66f, 0.25f, 1.0f),
    ImVec4(0.58f, 0.41f, 0.93f, 1.0f),
    ImVec4(0.18f, 0.80f, 0.78f, 1.0f),
};

void DrawControlPanel(VisualizerState* state, bool* needs_regenerate) {
    ImGui::Begin("Audio Buffer Visualizer");

    ImGui::Text("Adjust parameters to inspect adjacent buffers.");
    ImGui::Separator();

    *needs_regenerate |= ImGui::SliderFloat("Frequency (Hz)", &state->config.frequency_hz, 20.0f, 2000.0f);
    *needs_regenerate |= ImGui::SliderFloat("Amplitude", &state->config.amplitude, 0.0f, 1.0f);

    int sample_rate = state->config.sample_rate;
    if (ImGui::SliderInt("Sample Rate", &sample_rate, kMinSampleRate, kMaxSampleRate)) {
        sample_rate = std::max(sample_rate, kMinSampleRate);
        sample_rate = std::min(sample_rate, kMaxSampleRate);
        state->config.sample_rate = sample_rate;
        *needs_regenerate = true;
    }

    int frame_count = state->config.frame_count;
    if (ImGui::SliderInt("Buffer Length", &frame_count, kMinFrameCount, kMaxFrameCount)) {
        frame_count = std::max(frame_count, kMinFrameCount);
        frame_count = std::min(frame_count, kMaxFrameCount);
        state->config.frame_count = frame_count;
        *needs_regenerate = true;
    }

    if (ImGui::SliderInt("Buffers", &state->buffer_count, kMinBufferCount, kMaxBufferCount)) {
        *needs_regenerate = true;
    }

    *needs_regenerate |=
        ImGui::SliderFloat("Start Phase", &state->start_phase, 0.0f, kTwoPi);

    ImGui::Checkbox("Auto Refresh", &state->auto_refresh);
    ImGui::Checkbox("Show Concatenated View", &state->show_combined);

    if (ImGui::Button("Regenerate Buffers")) {
        *needs_regenerate = true;
    }

    ImGui::Separator();
    if (state->continuity_results.empty()) {
        ImGui::Text("Generate at least two buffers to inspect continuity.");
    } else {
        float max_diff = 0.0f;
        for (size_t i = 0; i < state->continuity_results.size(); ++i) {
            const auto& res = state->continuity_results[i];
            max_diff = std::max(max_diff, res.difference);
            ImGui::Text("Buffer %zu -> %zu: diff = %.6f (%s)",
                        i + 1, i + 2, res.difference,
                        res.is_continuous ? "ok" : "click detected");
        }
        ImGui::Separator();
        ImGui::Text("Max boundary delta: %.6f", max_diff);
    }

    DrawPlaybackSection(state);

    ImGui::End();
}

void DrawPlot(const VisualizerState& state) {
    ImGui::Begin("Buffer Plot");
    if (state.buffers.empty()) {
        ImGui::Text("No buffers generated yet.");
        ImGui::End();
        return;
    }

    const double total_samples =
        static_cast<double>(state.config.frame_count * state.buffer_count);

    if (ImPlot::BeginPlot("Phase Continuity", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Sample index", "Amplitude",
                          ImPlotAxisFlags_NoGridLines,
                          ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxesLimits(0.0, total_samples, -1.1, 1.1, ImPlotCond_Always);

        for (size_t i = 0; i < state.buffers.size(); ++i) {
            const auto& buffer = state.buffers[i];
            if (buffer.empty()) {
                continue;
            }
            const double x0 = static_cast<double>(i * state.config.frame_count);
            const auto color = kBufferColors[i % kBufferColors.size()];
            ImPlot::PushStyleColor(ImPlotCol_Line, color);
            std::string label = "Buffer " + std::to_string(i + 1);
            ImPlot::PlotLine(label.c_str(), buffer.data(),
                             static_cast<int>(buffer.size()), 1.0, x0);
            ImPlot::PopStyleColor();
        }

        if (state.show_combined && !state.concatenated.empty()) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));
            ImPlot::PlotLine("Concatenated", state.concatenated.data(),
                             static_cast<int>(state.concatenated.size()));
            ImPlot::PopStyleColor();
        }

        ImPlot::EndPlot();
    }

    ImGui::End();
}

}  // namespace

bool CreateDeviceD3D(HWND h_wnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

int main(int, char**) {
    WNDCLASSEXW wc = {sizeof(wc),       CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
                      nullptr,          nullptr,     nullptr, nullptr, L"AudioLoopViz",       nullptr};
    ::RegisterClassExW(&wc);
    HWND h_wnd = ::CreateWindowW(wc.lpszClassName,
                                 L"Audio Buffer Continuity Visualizer",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 1600, 900,
                                 nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(h_wnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(h_wnd, SW_SHOWDEFAULT);
    ::UpdateWindow(h_wnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(h_wnd);
    ImGui_ImplDX11_Init(g_pd3d_device, g_pd3d_device_context);

    VisualizerState state;
    state.config.frequency_hz = 220.0f;
    state.config.frame_count = 512;
    state.config.sample_rate = 48000;
    state.config.amplitude = 0.8f;
    RegenerateBuffers(&state);
    EnsureAudioInitialized(&state);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) {
            break;
        }

        if (g_swap_chain_occluded &&
            g_p_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_swap_chain_occluded = false;

        if (g_resize_width != 0 && g_resize_height != 0) {
            CleanupRenderTarget();
            g_p_swap_chain->ResizeBuffers(0, g_resize_width, g_resize_height,
                                          DXGI_FORMAT_UNKNOWN, 0);
            g_resize_width = g_resize_height = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        bool needs_regenerate = false;
        DrawControlPanel(&state, &needs_regenerate);
        DrawPlot(state);

        if (state.auto_refresh || needs_regenerate) {
            RegenerateBuffers(&state);
        }

        ImGui::Render();
        const float clear_color[4] = {0.05f, 0.05f, 0.08f, 1.0f};
        g_pd3d_device_context->OMSetRenderTargets(1, &g_main_render_target_view, nullptr);
        g_pd3d_device_context->ClearRenderTargetView(g_main_render_target_view,
                                                     clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_p_swap_chain->Present(1, 0);
        g_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(h_wnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND h_wnd) {
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
    const D3D_FEATURE_LEVEL feature_level_array[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                                                nullptr, create_device_flags,
                                                feature_level_array, 2,
                                                D3D11_SDK_VERSION, &sd, &g_p_swap_chain,
                                                &g_pd3d_device, &feature_level,
                                                &g_pd3d_device_context);
    if (res != S_OK) {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_p_swap_chain) {
        g_p_swap_chain->Release();
        g_p_swap_chain = nullptr;
    }
    if (g_pd3d_device_context) {
        g_pd3d_device_context->Release();
        g_pd3d_device_context = nullptr;
    }
    if (g_pd3d_device) {
        g_pd3d_device->Release();
        g_pd3d_device = nullptr;
    }
}

void CreateRenderTarget() {
    ID3D11Texture2D* p_back_buffer = nullptr;
    g_p_swap_chain->GetBuffer(0, IID_PPV_ARGS(&p_back_buffer));
    g_pd3d_device->CreateRenderTargetView(p_back_buffer, nullptr,
                                          &g_main_render_target_view);
    p_back_buffer->Release();
}

void CleanupRenderTarget() {
    if (g_main_render_target_view) {
        g_main_render_target_view->Release();
        g_main_render_target_view = nullptr;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND h_wnd, UINT msg,
                                                            WPARAM w_param,
                                                            LPARAM l_param);

LRESULT WINAPI WndProc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(h_wnd, msg, w_param, l_param)) {
        return true;
    }

    switch (msg) {
        case WM_SIZE:
            if (w_param == SIZE_MINIMIZED) {
                return 0;
            }
            g_resize_width = static_cast<UINT>(LOWORD(l_param));
            g_resize_height = static_cast<UINT>(HIWORD(l_param));
            return 0;
        case WM_SYSCOMMAND:
            if ((w_param & 0xfff0) == SC_KEYMENU) {
                return 0;
            }
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(h_wnd, msg, w_param, l_param);
}
