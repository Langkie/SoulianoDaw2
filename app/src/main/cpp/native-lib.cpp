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

// --- Static top-level Kotlin functions (MainActivityKt) ---
// The Compose lambdas call nativeStartStatic / nativeStopStatic / nativeToggleRecordStatic
// which are emitted into the MainActivityKt JVM class for top-level functions in MainActivity.kt.

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
