# Audio Loop Playground

This directory contains:

- `audio_loop_lib`: a phase-tracking oscillator (`PhaseContinuousSine`) that generates buffers of arbitrary length without introducing clicks between consecutive calls. Helper utilities expose continuity checks and buffer concatenation.
- `buffer_visualizer`: a DirectX11 + ImGui/ImPlot desktop viewer (derived from `//examples:imgui_dx11`) that plots multiple buffers side-by-side while you tune frequency, amplitude, sample rate, buffer length, starting phase, and buffer count. Continuity metrics are listed per buffer junction.
- `audio_buffer_test`: a gtest suite that validates smooth transitions across buffers, different buffer sizes, on-the-fly frequency changes, and detects intentional phase resets.

## Running the tests

Use `bazelisk` per the repo instructions:

```
bazelisk test //audio_loop:audio_buffer_test
```

## Launching the visualizer

```
bazelisk build //audio_loop:buffer_visualizer
bazel-bin/audio_loop/buffer_visualizer.exe
```

Use the control window to tweak parameters. Leave **Auto Refresh** enabled for live updates or disable it and click **Regenerate Buffers** for deterministic comparisons.
The **Audio Playback** section can stream the currently concatenated buffers through a simple XAudio2 player so you can listen for clicks while inspecting the plots.