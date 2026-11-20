# FARAZIQ Native Core — staged foundation

This initial implementation step adds four portable C++23 modules (four `.cpp` translation units):

1. **CPU Resource Manager** detects online/logical CPU capacity and returns a conservative worker-count recommendation. It does not alter Android's OS scheduler.
2. **GPU Resource Manager** selects the best backend reported by the Android/application adapter (Vulkan, OpenGL ES, CPU fallback) and tracks a strict app-provided reservation budget through RAII leases. It does not itself create a graphics context or submit GPU work.
3. **RAM Memory Pool** provides a fixed-capacity arena, aligned move-only RAII allocations, free-range coalescing, and usage/high-water statistics.
4. **Task Scheduler** runs jobs on a fixed worker pool with stable priority ordering, cooperative cancellation, futures for status delivery, exception containment, and cancellation of queued work during shutdown.

The repository currently has no Android app/module scaffold; the existing `My app` file is preserved. These modules are a first implementation step, not the complete FARAZIQ engine. Rendering, codecs/MediaCodec, timeline, storage, thermal monitoring, JNI, and actual Vulkan/OpenGL execution require later modules and Android lifecycle/context integration.

## Build and verification

With a C++23 compiler: `cmake -S . -B build && cmake --build build`.
For Android, configure through Android Studio's installed NDK toolchain and select the required ABI/minimum SDK in the consuming app. No external runtime dependency is used in this step. Compilation and Android-device execution have **not** been performed here; do not treat static authoring as a build result.

GPU capability values must be populated by a platform adapter that owns/probes a valid graphics instance/context. The GPU reservation API accounts for budget only. Scheduler jobs must periodically check their cancellation token; C++ cannot safely forcibly terminate a running task blocked in external code.
