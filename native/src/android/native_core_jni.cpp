#if defined(__ANDROID__)
#include <jni.h>

#include "faraziq/scheduler/task_scheduler.hpp"

#include <cstdint>
#include <new>

namespace {
struct NativeSession {
    explicit NativeSession(const std::size_t workers) : scheduler(workers) {}
    faraziq::scheduler::TaskScheduler scheduler;
};

jlong JNICALL native_create(JNIEnv* env, jclass, jint workers) noexcept {
    if (workers < 0 || workers > 256) return 0;
    try {
        auto* session = new NativeSession(static_cast<std::size_t>(workers));
        return static_cast<jlong>(reinterpret_cast<std::intptr_t>(session));
    } catch (const std::exception&) {
        if (env != nullptr) env->ExceptionClear();
        return 0;
    } catch (...) {
        return 0;
    }
}

void JNICALL native_destroy(JNIEnv*, jclass, jlong handle) noexcept {
    if (handle == 0) return;
    auto* session = reinterpret_cast<NativeSession*>(static_cast<std::intptr_t>(handle));
    delete session;
}

jstring JNICALL native_version(JNIEnv* env, jclass) noexcept {
    if (env == nullptr) return nullptr;
    return env->NewStringUTF("0.2.0");
}

JNINativeMethod methods[] = {
    {const_cast<char*>("nativeCreate"), const_cast<char*>("(I)J"), reinterpret_cast<void*>(native_create)},
    {const_cast<char*>("nativeDestroy"), const_cast<char*>("(J)V"), reinterpret_cast<void*>(native_destroy)},
    {const_cast<char*>("nativeGetVersion"), const_cast<char*>("()Ljava/lang/String;"), reinterpret_cast<void*>(native_version)},
};
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    if (vm == nullptr) return JNI_ERR;
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || env == nullptr) return JNI_ERR;
    jclass bridge = env->FindClass("com/faraziq/studio/NativeCore");
    if (bridge == nullptr) { env->ExceptionClear(); return JNI_ERR; }
    const jint result = env->RegisterNatives(bridge, methods, static_cast<jint>(sizeof(methods) / sizeof(methods[0])));
    env->DeleteLocalRef(bridge);
    if (result != JNI_OK) { env->ExceptionClear(); return JNI_ERR; }
    return JNI_VERSION_1_6;
}
#endif
