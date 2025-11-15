I'd like to create a library that can create audio at different frequencies.

The audio created is put into a buffer of arbitrary size, which will be passed to an audio interface to play via the speakers.

A common issue that is encountered is that the phase isn't maintained between buffers. For instance, if the first buffer and second buffer have a discontinuity between the end of the first and start of the second, this introduces a "clicking" sound that is heard every time a new buffer is started.

One way to get around this is to keep track of the Phase of the signal each time we create a new buffer, ensuring continuity between buffers.

I'd like to build a preliminary library for making these audio samples.

This will have 3 main parts:
1. Audio library with buffer generation
2. Imgui / implot draw code to visualize multiple buffers and ensure continuity with different frequencies, buffer lengths, and sample rates. (base this off //examples:imgui_dx11)
3. unit tests with gtest to ensure adjacent buffers are continuous sinusoidal waves.