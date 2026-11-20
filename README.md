# FARAZIQ Native Core — initial resource foundation

This repository adds a staged, portable C++23 native core and preserves the original `My app` file. Current implemented foundations cover CPU worker-capacity guidance, GPU backend policy and memory budget leases, bounded memory pools, priority task scheduling, pooled audio/video buffers, timeline metadata, thermal policy, CPU rendering adjustments, sandboxed project file I/O, and an Android JNI native-session bridge.

The system is offline-first and has no external runtime dependencies in the portable core. Platform capabilities must be supplied by Android adapters; GPU capability flags do not create a graphics context. Actual hardware rendering, MediaCodec, Android lifecycle/thermal callbacks, storage access framework, and complete editing features require additional integration.

## Build

Host C++23: `cmake -S . -B build && cmake --build build`.
Android: integrate the root CMake project with Android Studio/NDK and choose the app ABI and minimum SDK. If using the JNI bridge, implement `com.faraziq.studio.NativeCore` with matching native methods before loading the library.

Build and device tests have not been performed here; compile and test on the intended NDK/ABIs before production use. See `docs/IMPLEMENTATION_PROGRESS.md` for implemented scope, limitations, and verification status.
