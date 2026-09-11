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

// Static-style JNI wrappers used by Compose callbacks
extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeInitStatic(JNIEnv *env, jclass /* cls */) {
    if (!engine) engine = new AudioEngine();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStartStatic(JNIEnv *env, jclass /* cls */) {
    if (engine) engine->start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStopStatic(JNIEnv *env, jclass /* cls */) {
    if (engine) engine->stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeToggleRecordStatic(JNIEnv *env, jclass /* cls */, jboolean enable) {
    if (engine) engine->setRecording(enable);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStartRecordingStatic(JNIEnv *env, jclass /* cls */, jstring jpath) {
    if (!engine) return JNI_FALSE;
    const char* path = env->GetStringUTFChars(jpath, nullptr);
    bool ok = engine->startRecording(std::string(path));
    env->ReleaseStringUTFChars(jpath, path);
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStopRecordingStatic(JNIEnv *env, jclass /* cls */) {
    if (!engine) return;
    engine->stopRecording();
}
