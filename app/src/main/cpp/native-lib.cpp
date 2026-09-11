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
