# Bazel ImGui WebGL Project

A demonstration project showing how to build and run ImGui applications with Bazel, including both native Windows (DirectX 11) and WebGL (Emscripten) targets.


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

#### 2. ~~ImGui DirectX 11 (Windows)~~ (Removed)
```powershell
# Removed - requires backends not available in imgui module version
```
~~Native Windows ImGui application using DirectX 11 renderer.~~

#### 3. FLAC Library Test
```powershell
bazelisk build //examples:flac_test
bazelisk run //examples:flac_test
```
Tests the FLAC audio codec library integration.

---


## Running WebGL Applications

### Step 1: Build the Web Application

```powershell
bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm
```

### Step 2: Start the Web Server

```powershell
python serve.py
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

### Build Targets Summary

| Target | Description | Platform | Command |
|--------|-------------|----------|---------|
| `//examples:hello_world` | Abseil demo | Windows | `bazelisk build //examples:hello_world` |
| ~~`//examples:imgui_dx11`~~ | ~~ImGui DirectX 11~~ | ~~Windows~~ | Removed (backends unavailable) |
| `//examples:flac_test` | FLAC library test | Windows | `bazelisk build //examples:flac_test` |
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
python serve.py
```

### Watch for Changes

Bazel automatically tracks dependencies. Just rebuild after changes:

```powershell
bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm
```

The browser will use the updated files on refresh (Ctrl+F5 for hard refresh).


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
