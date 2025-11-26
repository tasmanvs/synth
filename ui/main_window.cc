#include "ui/main_window.h"
#include "imgui_internal.h"
#include "absl/log/log.h"
#include "absl/log/check.h"

MainWindow::MainWindow()
    : show_demo_window_(true)
    , show_implot_demo_window_(true)
    , show_audio_nodes_window_(true)
    , clear_color_(0.45f, 0.55f, 0.60f, 1.00f)
    , frequency_(440.0f)
    , volume_(0.3f)
    , playing_(false)
    , sample_rate_(44100)
    , dockspace_initialized_(false)
    , show_spectrogram_window_(true)
{
    LOG(INFO) << "MainWindow initialized with sample rate: " << sample_rate_;
    
    // Enable docking
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Initialize audio node graph
    audio_node_graph_ = std::make_unique<audio_nodes::AudioNodeGraph>(&audio_interface_);
}

MainWindow::~MainWindow()
{
}


void MainWindow::Update()
{
    // Update audio playback (legacy simple synth)
    static bool was_playing = false;
    static float last_frequency = 0.0f;
    static float last_volume = 0.0f;

    if (playing_ != was_playing)
    {
        if (playing_)
        {
            LOG(INFO) << "Starting audio tone at " << frequency_ << " Hz, volume " << volume_;
            
            // Note: Waveform generation removed - use audio node graph instead
            
            audio_interface_.Play();
        }
        else
        {
            LOG(INFO) << "Stopping audio tone";
            audio_interface_.Stop();
        }
        was_playing = playing_;
        last_frequency = frequency_;
        last_volume = volume_;
    }
    else if (playing_ && (frequency_ != last_frequency || volume_ != last_volume))
    {
        LOG(INFO) << "Updating tone: frequency=" << frequency_ << " Hz, volume=" << volume_;
        
        // Note: Waveform generation removed - use audio node graph instead
        
        last_frequency = frequency_;
        last_volume = volume_;
    }
    
    // Update audio node graph
    if (audio_node_graph_) {
        audio_node_graph_->Update(sample_rate_);
    }
}

void MainWindow::Draw()
{
    // Create dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    
    // Initialize dockspace layout on first frame
    if (!dockspace_initialized_) {
        dockspace_initialized_ = true;
        
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);
        
        // Split: top 80% and bottom 20%
        ImGuiID dock_top = 0;
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.20f, nullptr, &dock_top);
        
        // Dock windows to top (these will be tabbed)
        ImGui::DockBuilderDockWindow("Audio Nodes", dock_top);
        ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_top);
        ImGui::DockBuilderDockWindow("ImPlot Demo", dock_top);
        
        // Dock spectrogram to bottom
        ImGui::DockBuilderDockWindow("Spectrogram", dock_bottom);
        
        ImGui::DockBuilderFinish(dockspace_id);
        
        // Set Audio Nodes window to be focused on startup
        ImGui::SetWindowFocus("Audio Nodes");
    }
    
    ImGui::End();
    
    // 1. Show the big demo window
    if (show_demo_window_)
        ImGui::ShowDemoWindow(&show_demo_window_);

    // 2. Show the ImPlot demo window
    if (show_implot_demo_window_)
        ImPlot::ShowDemoWindow(&show_implot_demo_window_);

    // 3. Show audio nodes window
    if (show_audio_nodes_window_)
        DrawAudioNodesWindow();
    
    // 7. Show spectrogram window
    if (show_spectrogram_window_)
        DrawSpectrogramWindow();

}

void MainWindow::DrawAudioNodesWindow()
{
    ImGui::Begin("Audio Nodes", &show_audio_nodes_window_, ImGuiWindowFlags_MenuBar);
    
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Source Node")) {
                audio_node_graph_->CreateSourceNode();
            }
            if (ImGui::MenuItem("Sum Node")) {
                audio_node_graph_->CreateSumNode();
            }
            if (ImGui::MenuItem("Player Node")) {
                audio_node_graph_->CreatePlayerNode();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Right-click background to create nodes", nullptr, false, false);
            ImGui::MenuItem("Drag from output (Out ->) to input (-> In)", nullptr, false, false);
            ImGui::MenuItem("Right-click link or node to delete", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    ImGui::TextWrapped("Create audio nodes and connect them to make sound!");
    ImGui::Separator();
    
    // Draw the audio node graph
    if (audio_node_graph_) {
        audio_node_graph_->Draw();
    }
    
    ImGui::End();
}

void MainWindow::DrawSpectrogramWindow()
{
    ImGui::Begin("Spectrogram", &show_spectrogram_window_);
    
    // Find the player node and delegate to its spectrogram view
    if (audio_node_graph_) {
        const auto& nodes = audio_node_graph_->GetNodes();
        audio_nodes::PlayerNode* player_node = nullptr;
        
        for (const auto& [node_id, node] : nodes) {
            if (node->GetNodeType() == audio_nodes::NodeType::kPlayer) {
                player_node = static_cast<audio_nodes::PlayerNode*>(node.get());
                break;
            }
        }
        
        if (player_node) {
            player_node->DrawSpectrogramContent();
        } else {
            ImGui::TextWrapped("No player node found. Create a player node in the Audio Nodes window to see the spectrogram.");
        }
    } else {
        ImGui::Text("Audio node graph not initialized.");
    }
    
    ImGui::End();
}
