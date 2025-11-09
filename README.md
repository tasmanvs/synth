# Bazel ImGui WebGL Project

A demonstration project showing how to build and run ImGui applications with Bazel, including both native Windows (DirectX 11) and WebGL (Emscripten) targets.

## Prerequisites

- **Bazelisk** - Already installed in `%LOCALAPPDATA%\bazelisk\`
- **Visual Studio Build Tools 2022** - With "Desktop development with C++" workload
- **Python 3.12** - For running the local web server

## Project Structure

```
bazel_testing/
├── hello_world.cc          # Simple Abseil hello world
├── imgui_dx11.cc           # ImGui with DirectX 11 (Windows native)
├── imgui_webgl.cc          # ImGui with WebGL (Emscripten)
├── main_window.h/cc        # Main UI window class
├── backends/               # Local ImGui backend files
│   ├── imgui_impl_glfw.*
│   └── imgui_impl_opengl3.*
├── shell.html              # HTML template for WebGL
├── serve.py                # Python HTTP server
├── run_webgl.ps1           # Build and run script
├── BUILD                   # Bazel build configuration
├── MODULE.bazel            # Bazel module dependencies
└── WORKSPACE               # Workspace configuration (Emscripten)
```

## Building

### 1. Build Native Windows Application (DirectX 11)

```powershell
bazelisk build //:imgui_hello
```

**Run:**
```powershell
bazelisk run //:imgui_hello
# Or directly:
.\bazel-bin\imgui_hello.exe
```

### 2. Build Hello World (Abseil example)

```powershell
bazelisk build //:hello_world
bazelisk run //:hello_world
```

### 3. Build FLAC Test

```powershell
bazelisk build //:flac_test
bazelisk run //:flac_test
```

### 4. Build WebGL Application (Emscripten)

```powershell
bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm
```

This generates:
- `bazel-bin/imgui_webgl.js` - JavaScript glue code (~335 KB)
- `bazel-bin/imgui_webgl.wasm` - WebAssembly binary (~2.1 MB)
- `bazel-bin/imgui_webgl.html` - HTML launcher

## Running the WebGL Application

### Option 1: Use the Convenience Script (Recommended)

```powershell
powershell -ExecutionPolicy Bypass -File .\run_webgl.ps1
```

This will:
1. Build the WebGL target
2. Compress files with gzip
3. Start an HTTP server
4. Open your browser automatically

### Option 2: Manual Method

```powershell
# Build first
bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm

# Start Python server
py serve.py

# Open browser to http://localhost:8080/imgui_webgl.html
```

### Option 3: Python Built-in Server

```powershell
cd bazel-bin
py -m http.server 8080
# Then navigate to http://localhost:8080/imgui_webgl.html
```

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

### Build Targets

| Target | Description | Platform |
|--------|-------------|----------|
| `hello_world` | Abseil demo | Windows |
| `imgui_hello` | ImGui DirectX 11 | Windows |
| `imgui_webgl` | ImGui + ImPlot WebGL | Browser (WASM) |
| `flac_test` | FLAC library test | Windows |

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
2. Check browser console for errors (F12)
3. Verify files exist in `bazel-bin/`:
   - `imgui_webgl.html`
   - `imgui_webgl.js`
   - `imgui_webgl.wasm`

## Performance

- **Uncompressed WASM:** ~2.1 MB
- **Gzip compressed:** ~500-600 KB (70% reduction)
- **JavaScript:** ~335 KB uncompressed
- **First load:** 2-5 seconds (depending on network)
- **Subsequent loads:** Instant (browser cache)

## Development Tips

### Iterate Quickly

```powershell
# Build and run in one command
bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm && py serve.py
```

### Watch for Changes

Bazel automatically tracks dependencies. Just rebuild after changes:

```powershell
bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm
```

The browser will use the updated files on refresh.

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
