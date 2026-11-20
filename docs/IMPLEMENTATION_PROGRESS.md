# FARAZIQ Native Core — progress

## Implemented modules

1. CPU logical-capacity detection and conservative worker recommendations.
2. GPU backend policy (Vulkan/OpenGL ES/CPU fallback) and bounded RAII reservation accounting. Graphics device/context creation and command execution require a platform graphics adapter.
3. Bounded RAM arena with aligned move-only RAII buffers.
4. Fixed-worker priority scheduler with cancellation, future status reporting, and exception containment.
5. Pooled video/audio buffer types with dimension and allocation-overflow validation.
6. Thread-safe timeline track/clip metadata operations and duration queries.
7. Thermal/memory-pressure policy mapping to job concurrency and GPU allowance.
8. CPU software renderer implementing exposure, invert, and grayscale operations over validated frames; alpha is preserved for RGBA.
9. Sandboxed project file read and atomic write utility rooted in an app-private directory.
10. Android JNI registration bridge that creates/destroys a scheduler-backed native session and reports version.

## Current scope and remaining work

These are concrete foundations, not the complete professional editing engine. MediaCodec decoding/encoding, actual Vulkan/OpenGL render backends, Android thermal/memory callbacks, SAF integration, project serialization/recovery, undo/redo, engine plugin registration, audio DSP, export coordination, and diagnostics remain unimplemented. Android Kotlin/Java must provide class `com.faraziq.studio.NativeCore` with native methods `nativeCreate(int): long`, `nativeDestroy(long): void`, and `nativeGetVersion(): String`, and load the built library. The project began with no Android app scaffold; the existing `My app` file is preserved.

## Build and verification

CMake targets C++23. No compiler/build runner or Android device was used for this commit; compilation and runtime behavior are unverified. For a host build run `cmake -S . -B build && cmake --build build` using a C++23 compiler. For Android, import the root CMake project through Android Studio, use the installed NDK, and configure the app's ABI/minimum SDK. Android JNI code is included only when `ANDROID` is true. Do not interpret the source additions as a successful build or device test.
