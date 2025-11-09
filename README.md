# Bazel ImGui WebGL Project

A demonstration project showing how to build and run ImGui applications with Bazel, including both native Windows (DirectX 11) and WebGL (Emscripten) targets.

## Prerequisites

- **Bazelisk** - Already installed in `%LOCALAPPDATA%\bazelisk\`
- **Visual Studio Build Tools 2022** - With "Desktop development with C++" workload
- **Python 3.12** - For running the local web server

## Project Structure

```
bazel_testing/
├── apps/                   # Main application targets
│   ├── BUILD
│   └── imgui_webgl.cc      # ImGui WebGL application
├── audio/                  # Audio synthesis and playback
│   ├── BUILD
│   ├── audio_synth.*       # Pure waveform generator
│   ├── audio_interface.*   # OpenAL audio playback
│   └── audio*.js           # Web Audio JavaScript
├── ui/                     # UI components
│   ├── BUILD
│   ├── main_window.*       # Main UI window class
│   └── implot_utils.*      # ImPlot utilities
├── examples/               # Demo/example programs
│   ├── BUILD
│   ├── hello_world.cc      # Simple Abseil demo
│   ├── imgui_dx11.cc       # ImGui DirectX 11 (Windows)
│   └── flac_test.cc        # FLAC library test
├── backends/               # ImGui backend implementations
│   ├── imgui_impl_glfw.*
│   └── imgui_impl_opengl3.*
├── third_party/            # Third-party build configs
├── shell.html              # HTML template for WebGL
├── serve.py                # Python HTTP server
├── run_webgl.ps1           # Build and run script
├── BUILD                   # Root build file
├── MODULE.bazel            # Bazel module dependencies
└── WORKSPACE               # Workspace configuration
```

## Available Build Targets

### Build All Compatible Targets

```powershell
# Build all targets compatible with your platform (Windows)
bazelisk build //...
```

This builds all Windows-native targets and skips web/WASM targets automatically.

---

### Examples (Windows Native)

#### 1. Hello World (Abseil Demo)
```powershell
bazelisk build //examples:hello_world
bazelisk run //examples:hello_world
```
Simple demonstration of Abseil string utilities and logging.

#### 2. ImGui DirectX 11 (Windows)
```powershell
bazelisk build //examples:imgui_dx11
bazelisk run //examples:imgui_dx11
```
Native Windows ImGui application using DirectX 11 renderer.

#### 3. FLAC Library Test
```powershell
bazelisk build //examples:flac_test
bazelisk run //examples:flac_test
```
Tests the FLAC audio codec library integration.

---

### Libraries

#### Audio Synthesis Library
```powershell
bazelisk build //audio:audio_synth
```
Pure waveform generator library (no platform dependencies).

---

### Web Applications (Require Emscripten)

#### ImGui WebGL Application
```powershell
bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm
```
Full ImGui + ImPlot application compiled to WebAssembly.

**Output files:** (in `bazel-bin/apps/`)
- `imgui_webgl.js` - JavaScript glue code (~443 KB)
- `imgui_webgl.wasm` - WebAssembly binary (~4.2 MB)
- Use with `shell.html` as the HTML launcher

#### Audio WebGL Application
```powershell
bazelisk build //apps:audio_webgl --platforms=@emsdk//:platform_wasm
```
ImGui application with OpenAL audio support for the web.

---

### Web Targets Requiring OpenGL/GLFW (Emscripten only)

#### ImGui OpenGL Demo
```powershell
bazelisk build //examples:imgui_hello --platforms=@emsdk//:platform_wasm
```
OpenGL-based ImGui demo (GLFW + OpenGL ES3).

## Running WebGL Applications

### Step 1: Build the Web Application

```powershell
bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm
```

### Step 2: Verify Output Files

The build should generate (in `bazel-bin/apps/`):
- `imgui_webgl.js` - JavaScript glue code
- `imgui_webgl.wasm` - WebAssembly binary  
- `imgui_webgl` - Main binary (uses shell.html via --shell-file)

**Note:** If HTML is not generated, copy manually:
```powershell
Copy-Item shell.html bazel-bin\apps\imgui_webgl.html
```

### Step 3: Start the Web Server

```powershell
python serve.py
```

The server will automatically:
- Serve from `bazel-bin/apps/` (new structure)
- List all available HTML files
- Set proper CORS headers for WebAssembly
- Open at http://localhost:8080/

### Alternative: Use the Convenience Script

```powershell
powershell -ExecutionPolicy Bypass -File .\run_webgl.ps1
```

**Note:** This script may need updating for the new directory structure.

## Dependencies

### External Dependencies (managed by Bazel)

- **abseil-cpp** (20240116.2) - C++ utility library
- **imgui** (1.91.8) - Immediate mode GUI library
- **implot** (0.16) - Plotting library for ImGui
- **emsdk** (3.1.56) - Emscripten SDK for WebAssembly
- **flac** (1.5.0) - Free Lossless Audio Codec library

### System Requirements

- **Windows 10/11**
- **Visual Studio Build Tools 2022** with MSVC v143
- **Python 3.12+**

## Architecture

### MainWindow Class

The UI is organized using a `MainWindow` class with two methods:

- `Update()` - Per-frame logic updates
- `Draw()` - ImGui rendering calls

This separation keeps the UI code modular and testable.

### Build Targets Summary

| Target | Description | Platform | Command |
|--------|-------------|----------|---------|
| `//examples:hello_world` | Abseil demo | Windows | `bazelisk build //examples:hello_world` |
| `//examples:imgui_dx11` | ImGui DirectX 11 | Windows | `bazelisk build //examples:imgui_dx11` |
| `//examples:flac_test` | FLAC library test | Windows | `bazelisk build //examples:flac_test` |
| `//audio:audio_synth` | Audio synthesis lib | All | `bazelisk build //audio:audio_synth` |
| `//apps:imgui_webgl` | ImGui + ImPlot WebGL | Browser (WASM) | `bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm` |
| `//apps:audio_webgl` | ImGui + OpenAL WebGL | Browser (WASM) | `bazelisk build //apps:audio_webgl --platforms=@emsdk//:platform_wasm` |
| `//examples:imgui_hello` | ImGui OpenGL demo | Browser (WASM) | `bazelisk build //examples:imgui_hello --platforms=@emsdk//:platform_wasm` |

## Features

### WebGL Build Features

- ✅ WebGL 2.0 with OpenGL ES 3
- ✅ GLFW for windowing
- ✅ Exception handling enabled
- ✅ Assertions for debugging
- ✅ Memory growth allowed
- ✅ Custom HTML shell with error handling
- ✅ Proper MIME types and CORS headers
- ✅ Responsive design with viewport meta tag

### Error Handling

The WebGL build includes comprehensive error handling:
- Early WebGL 2 capability detection
- WebGL context loss/restore handlers
- Global error catching with UI feedback
- Page visibility change handlers

## Troubleshooting

### Python Not Found

If you get "Python was not found":

1. **Disable Windows Store Python alias:**
   - Settings → Apps → Advanced app settings → App execution aliases
   - Toggle OFF: `python.exe` and `python3.exe`

2. **Use `py` launcher instead:**
   ```powershell
   py serve.py
   ```

### Port Already in Use

If port 8080 is busy:

```powershell
# Find and kill process
Get-NetTCPConnection -LocalPort 8080 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }
```

Or edit `serve.py` to use a different port.

### Build Errors

Clear Bazel cache:
```powershell
bazelisk clean --expunge
```

### WebGL Not Loading

1. Ensure you built with the correct platform flag: `--platforms=@emsdk//:platform_wasm`
2. Verify files exist in `bazel-bin/apps/`:
   - `imgui_webgl.js` (generated)
   - `imgui_webgl.wasm` (generated)
   - `imgui_webgl.html` (auto-generated or copy from shell.html)
3. If HTML not present: `Copy-Item shell.html bazel-bin\apps\imgui_webgl.html`
4. Check browser console for errors (F12)
5. Ensure server has proper CORS headers (use serve.py)

## Performance

- **Uncompressed WASM:** ~4.2 MB (imgui_webgl)
- **Gzip compressed:** ~800 KB - 1 MB (70-75% reduction)
- **JavaScript:** ~443 KB uncompressed
- **First load:** 2-5 seconds (depending on network)
- **Subsequent loads:** Instant (browser cache)

**Note:** File sizes may vary based on build configuration and included features.

## Development Tips

### Iterate Quickly

```powershell
# Build and serve web app in one command (if HTML not auto-generated)
bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm
if (-not (Test-Path bazel-bin\apps\imgui_webgl.html)) { Copy-Item shell.html bazel-bin\apps\imgui_webgl.html }
python serve.py
```

### Watch for Changes

Bazel automatically tracks dependencies. Just rebuild after changes:

```powershell
bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm
```

The browser will use the updated files on refresh (Ctrl+F5 for hard refresh).

### Debug Build

Add these flags for more detailed error messages:

```powershell
bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm -c dbg
```

## License

This is a demonstration project. Dependencies have their own licenses:
- ImGui: MIT License
- Abseil: Apache 2.0 License
- Emscripten: MIT License

## Resources

- [Bazel Documentation](https://bazel.build/)
- [ImGui Repository](https://github.com/ocornut/imgui)
- [Emscripten Documentation](https://emscripten.org/)
- [Bazel Central Registry](https://registry.bazel.build/)
