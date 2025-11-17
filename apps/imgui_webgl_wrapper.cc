#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define ACCESS _access
#else
#include <unistd.h>
#define ACCESS access
#endif

namespace {

std::string GetEnv(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

std::string QuotePath(const std::string& path) {
    if (path.empty()) {
        return "\"\"";
    }
    if (path.front() == '"' && path.back() == '"') {
        return path;
    }
    return "\"" + path + "\"";
}

bool FileExists(const std::string& path) {
    return ACCESS(path.c_str(), 0) == 0;
}

int RunCommand(const std::string& command) {
    std::cout << "[wrapper] executing: " << command << std::endl;
    int code = std::system(command.c_str());
    if (code != 0) {
        std::cerr << "[wrapper] command failed with exit code " << code << std::endl;
    }
    return code;
}

bool EnsureBuild(const std::string& workspace) {
    const std::string build_cmd = "bazelisk build //apps:imgui_webgl_bin //apps:imgui_webgl_html --platforms=@emsdk//:platform_wasm";
#ifdef _WIN32
    std::string inner = "cd /D " + QuotePath(workspace) + " && " + build_cmd;
    std::string cmd = "cmd /C \"" + inner + "\"";
#else
    std::string inner = "cd " + QuotePath(workspace) + " && " + build_cmd;
    std::string cmd = "bash -c \"" + inner + "\"";
#endif
    return RunCommand(cmd) == 0;
}

bool ServeDirectory(const std::string& directory, int port) {
    std::string command;
#ifdef _WIN32
    std::string inner = "cd /D " + QuotePath(directory) + " && python -m http.server " + std::to_string(port);
    command = "cmd /C \"" + inner + "\"";
#else
    std::string inner = "cd " + QuotePath(directory) + " && python3 -m http.server " + std::to_string(port);
    command = "bash -c \"" + inner + "\"";
#endif
    std::cout << "[wrapper] serving " << directory << " on http://127.0.0.1:" << port << std::endl;
    std::cout << "[wrapper] press Ctrl+C to stop the server" << std::endl;
    return RunCommand(command) == 0;
}

}  // namespace

std::string CurrentWorkspaceFallback() {
#ifdef _WIN32
    constexpr int kBufferSize = 4096;
    char buffer[kBufferSize];
    if (_getcwd(buffer, kBufferSize)) {
        return std::string(buffer);
    }
#else
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer))) {
        return std::string(buffer);
    }
#endif
    return std::string(".");
}

std::string BuildOutputPath(const std::string& workspace, const std::string& file_name) {
#ifdef _WIN32
    return workspace + "\\bazel-bin\\apps\\" + file_name;
#else
    return workspace + "/bazel-bin/apps/" + file_name;
#endif
}

std::string FindOutputHtml(const std::string& workspace) {
    const char* const candidates[] = {
        "imgui_webgl_bin.html",
        "imgui_webgl_index.html",
        "imgui_webgl_generated.html",
    };
    for (const char* candidate : candidates) {
        const std::string path = BuildOutputPath(workspace, candidate);
        if (FileExists(path)) {
            return path;
        }
    }
    return std::string();
}

std::string GetServeDirectory(const std::string& html_file) {
    size_t pos = html_file.find_last_of("/\\");
    if (pos == std::string::npos) {
        return ".";
    }
    return html_file.substr(0, pos);
}

int main(int /*argc*/, char** /*argv*/) {
    std::string workspace = GetEnv("BUILD_WORKSPACE_DIRECTORY");
    if (workspace.empty()) {
        workspace = CurrentWorkspaceFallback();
        std::cerr << "[wrapper] BUILD_WORKSPACE_DIRECTORY not set; using current path: " << workspace << std::endl;
    }

    if (!EnsureBuild(workspace)) {
        std::cerr << "[wrapper] failed to build WebGL targets (//apps:imgui_webgl_bin and //apps:imgui_webgl_html)" << std::endl;
        return 1;
    }

    std::string html_file = FindOutputHtml(workspace);
    if (html_file.empty()) {
        std::cerr << "[wrapper] could not locate generated HTML (expected imgui_webgl_bin.html, imgui_webgl_index.html, or imgui_webgl_generated.html)" << std::endl;
        return 1;
    }

    std::string output_dir = GetServeDirectory(html_file);
    const int port = 8000;
    if (!ServeDirectory(output_dir, port)) {
        return 1;
    }

    return 0;
}
