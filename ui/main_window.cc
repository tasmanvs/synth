#include "ui/main_window.h"
#include <cmath>
#include <algorithm>
#include "absl/log/log.h"
#include "absl/log/check.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MainWindow::MainWindow()
    : show_demo_window_(true)
    , show_implot_demo_window_(true)
    , show_another_window_(false)
    , show_node_editor_window_(false)
    , show_audio_nodes_window_(true)
    , clear_color_(0.45f, 0.55f, 0.60f, 1.00f)
    , frequency_(440.0f)
    , volume_(0.3f)
    , playing_(false)
    , sample_rate_(44100)
    , node_editor_context_(nullptr)
    , node_editor_initialized_(false)
{
    LOG(INFO) << "MainWindow initialized with sample rate: " << sample_rate_;
    
    // Enable docking
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Initialize node editor (for demo)
    ax::NodeEditor::Config config;
    node_editor_context_ = ax::NodeEditor::CreateEditor(&config);
    
    // Initialize audio node graph
    audio_node_graph_ = std::make_unique<audio_nodes::AudioNodeGraph>(&audio_interface_);
}

MainWindow::~MainWindow()
{
    if (node_editor_context_)
    {
        ax::NodeEditor::DestroyEditor(node_editor_context_);
        node_editor_context_ = nullptr;
    }
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
    ImGui::End();
    
    // 1. Show the big demo window
    if (show_demo_window_)
        ImGui::ShowDemoWindow(&show_demo_window_);

    // 2. Show the ImPlot demo window
    if (show_implot_demo_window_)
        ImPlot::ShowDemoWindow(&show_implot_demo_window_);

    // 3. Show a simple window
    {
        ImGui::Begin("Hello, Bazel + ImGui + WebGL! V2");

        ImGui::Text("This is ImGui running in a web browser with WebGL!");
        ImGui::Text("Built with Bazel and Emscripten!");
        ImGui::Checkbox("ImGui Demo Window", &show_demo_window_);
        ImGui::Checkbox("ImPlot Demo Window", &show_implot_demo_window_);
        ImGui::Checkbox("Node Editor Demo", &show_node_editor_window_);
        ImGui::Checkbox("Audio Nodes Window", &show_audio_nodes_window_);
        ImGui::Checkbox("Another Window", &show_another_window_);

        ImGui::Separator();
        ImGui::ColorEdit3("clear color", (float*)&clear_color_);


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
    
    // 5. Show node editor demo
    if (show_node_editor_window_)
        DrawNodeEditorDemo();
    
    // 6. Show audio nodes window
    if (show_audio_nodes_window_)
        DrawAudioNodesWindow();

}

void MainWindow::DrawNodeEditorDemo()
{
    ImGui::Begin("Node Editor Demo", &show_node_editor_window_, ImGuiWindowFlags_None);
    ImGui::TextWrapped("Controls:");
    ImGui::BulletText("Drag nodes with left mouse button");
    ImGui::BulletText("Drag from output pin (Out ->) to input pin (-> In) to create a link");
    ImGui::BulletText("Right-click on link to delete it");
    ImGui::BulletText("Mouse wheel to zoom");
    ImGui::BulletText("Press 'F' to fit all nodes in view");
    ImGui::Separator();
    
    namespace ed = ax::NodeEditor;
    
    ed::SetCurrentEditor(node_editor_context_);
    
    // Use a larger canvas size
    ed::Begin("Node Editor", ImVec2(0.0f, 0.0f));
    
    int unique_id = 1;
    
    // Node 1: Input Node
    ed::BeginNode(unique_id++);
        ImGui::Text("Input Node");
        ed::BeginPin(unique_id++, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
    ed::EndNode();
    
    // Node 2: Processing Node
    ed::BeginNode(unique_id++);
        ImGui::Text("Audio Processor");
        ed::BeginPin(unique_id++, ed::PinKind::Input);
            ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(unique_id++, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
    ed::EndNode();
    
    // Node 3: Output Node
    ed::BeginNode(unique_id++);
        ImGui::Text("Output Node");
        ed::BeginPin(unique_id++, ed::PinKind::Input);
            ImGui::Text("-> In");
        ed::EndPin();
    ed::EndNode();
    
    // Node 4: Parameter Node
    ed::BeginNode(unique_id++);
        ImGui::Text("Parameters");
        static float gain = 0.5f;
        ImGui::SliderFloat("Gain", &gain, 0.0f, 1.0f);
        ed::BeginPin(unique_id++, ed::PinKind::Output);
            ImGui::Text("Value ->");
        ed::EndPin();
    ed::EndNode();
    
    // Set initial positions only once
    if (!node_editor_initialized_)
    {
        ed::SetNodePosition(1, ImVec2(50.0f, 50.0f));
        ed::SetNodePosition(3, ImVec2(300.0f, 50.0f));
        ed::SetNodePosition(6, ImVec2(550.0f, 50.0f));
        ed::SetNodePosition(8, ImVec2(300.0f, 200.0f));
        ed::NavigateToContent(0.0f);
        node_editor_initialized_ = true;
    }
    
    // Store created links
    struct Link
    {
        ed::LinkId id;
        ed::PinId start_pin_id;
        ed::PinId end_pin_id;
    };
    static std::vector<Link> links;
    static int next_link_id = 1000;
    
    // Draw existing links
    for (const auto& link : links)
    {
        ed::Link(link.id, link.start_pin_id, link.end_pin_id);
    }
    
    // Handle link creation
    if (ed::BeginCreate())
    {
        ed::PinId start_pin_id, end_pin_id;
        if (ed::QueryNewLink(&start_pin_id, &end_pin_id))
        {
            if (start_pin_id && end_pin_id)
            {
                if (ed::AcceptNewItem())
                {
                    // Create and store the new link
                    links.push_back(Link{ed::LinkId(next_link_id++), start_pin_id, end_pin_id});
                }
            }
        }
    }
    ed::EndCreate();
    
    // Handle link deletion
    if (ed::BeginDelete())
    {
        ed::LinkId deleted_link_id;
        while (ed::QueryDeletedLink(&deleted_link_id))
        {
            if (ed::AcceptDeletedItem())
            {
                // Remove the link from our vector
                links.erase(
                    std::remove_if(links.begin(), links.end(),
                        [deleted_link_id](const Link& link) { return link.id == deleted_link_id; }),
                    links.end()
                );
            }
        }
    }
    ed::EndDelete();
    
    ed::End();
    ed::SetCurrentEditor(nullptr);
    
    ImGui::End();
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
