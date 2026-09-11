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

// --- Sample playback methods ---
extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeLoadSample(JNIEnv *env, jobject /* this */, jstring filePath) {
    if (!engine) return JNI_FALSE;
    const char* path = env->GetStringUTFChars(filePath, nullptr);
    jboolean result = engine->loadSample(path) ? JNI_TRUE : JNI_FALSE;
    env->ReleaseStringUTFChars(filePath, path);
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativePlaySample(JNIEnv *env, jobject /* this */) {
    if (engine) engine->playSample();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeStopSample(JNIEnv *env, jobject /* this */) {
    if (engine) engine->stopSample();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeSeekSample(JNIEnv *env, jobject /* this */, jdouble timeSeconds) {
    if (engine) engine->seekSample(timeSeconds);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeGetSamplePosition(JNIEnv *env, jobject /* this */) {
    if (!engine) return 0.0;
    return engine->getSamplePosition();
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeGetSampleDuration(JNIEnv *env, jobject /* this */) {
    if (!engine) return 0.0;
    return engine->getSampleDuration();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeHasSampleLoaded(JNIEnv *env, jobject /* this */) {
    if (!engine) return JNI_FALSE;
    return engine->hasSampleLoaded() ? JNI_TRUE : JNI_FALSE;
}

// --- Static top-level Kotlin functions (MainActivityKt) ---
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

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeLoadSampleStatic(JNIEnv *env, jclass /* clazz */, jstring filePath) {
    if (!engine) return JNI_FALSE;
    const char* path = env->GetStringUTFChars(filePath, nullptr);
    jboolean result = engine->loadSample(path) ? JNI_TRUE : JNI_FALSE;
    env->ReleaseStringUTFChars(filePath, path);
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativePlaySampleStatic(JNIEnv *env, jclass /* clazz */) {
    if (engine) engine->playSample();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeStopSampleStatic(JNIEnv *env, jclass /* clazz */) {
    if (engine) engine->stopSample();
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeSeekSampleStatic(JNIEnv *env, jclass /* clazz */, jdouble timeSeconds) {
    if (engine) engine->seekSample(timeSeconds);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeGetSamplePositionStatic(JNIEnv *env, jclass /* clazz */) {
    if (!engine) return 0.0;
    return engine->getSamplePosition();
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeGetSampleDurationStatic(JNIEnv *env, jclass /* clazz */) {
    if (!engine) return 0.0;
    return engine->getSampleDuration();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivityKt_nativeHasSampleLoadedStatic(JNIEnv *env, jclass /* clazz */) {
    if (!engine) return JNI_FALSE;
    return engine->hasSampleLoaded() ? JNI_TRUE : JNI_FALSE;
}
