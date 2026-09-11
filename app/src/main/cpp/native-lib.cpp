#include <jni.h>
#include <string>
#include "AudioEngine.h"

static AudioEngine* engine = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeInit(JNIEnv *env, jobject /* this */) {
    if (!engine) engine = new AudioEngine();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStart(JNIEnv *env, jobject /* this */) {
    if (engine) engine->start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStop(JNIEnv *env, jobject /* this */) {
    if (engine) engine->stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeToggleRecord(JNIEnv *env, jobject /* this */, jboolean enable) {
    if (engine) engine->setRecording(enable);
}

// Static top-level Kotlin functions (MainActivityKt)
extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeInitStatic(JNIEnv *env, jclass /* clazz */) {
    if (!engine) engine = new AudioEngine();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeStartStatic(JNIEnv *env, jclass /* clazz */) {
    if (engine) engine->start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeStopStatic(JNIEnv *env, jclass /* clazz */) {
    if (engine) engine->stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeToggleRecordStatic(JNIEnv *env, jclass /* clazz */, jboolean enable) {
    if (engine) engine->setRecording(enable);
}

// New JNI functions for start/stop recording with path
extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStartRecording(JNIEnv *env, jobject /* this */, jstring path) {
    if (!engine) return JNI_FALSE;
    const char *cpath = env->GetStringUTFChars(path, nullptr);
    bool res = engine->startRecording(std::string(cpath));
    env->ReleaseStringUTFChars(path, cpath);
    return res ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStopRecording(JNIEnv *env, jobject /* this */) {
    if (engine) engine->stopRecording();
}

// top-level static variants
extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeStartRecordingStatic(JNIEnv *env, jclass /* clazz */, jstring path) {
    if (!engine) return JNI_FALSE;
    const char *cpath = env->GetStringUTFChars(path, nullptr);
    bool res = engine->startRecording(std::string(cpath));
    env->ReleaseStringUTFChars(path, cpath);
    return res ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeStopRecordingStatic(JNIEnv *env, jclass /* clazz */) {
    if (engine) engine->stopRecording();
}
