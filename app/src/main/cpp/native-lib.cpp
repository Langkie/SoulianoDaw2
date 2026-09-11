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

// Sample load / trigger
extern "C" JNIEXPORT jint JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeLoadSampleStatic(JNIEnv *env, jclass /* cls */, jstring jpath) {
    if (!engine) return -1;
    const char* path = env->GetStringUTFChars(jpath, nullptr);
    int id = engine->loadSample(std::string(path));
    env->ReleaseStringUTFChars(jpath, path);
    return id;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeTriggerSampleStatic(JNIEnv *env, jclass /* cls */, jint sampleId) {
    if (!engine) return JNI_FALSE;
    bool ok = engine->triggerSample(static_cast<int>(sampleId));
    return ok ? JNI_TRUE : JNI_FALSE;
}

// Track management
extern "C" JNIEXPORT jint JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeCreateTrackWithSampleStatic(JNIEnv *env, jclass /* cls */, jstring jpath) {
    if (!engine) return -1;
    const char* path = env->GetStringUTFChars(jpath, nullptr);
    int id = engine->createTrackWithSample(std::string(path));
    env->ReleaseStringUTFChars(jpath, path);
    return id;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeSetTrackGainStatic(JNIEnv *env, jclass /* cls */, jint trackId, jfloat gain) {
    if (!engine) return JNI_FALSE;
    bool ok = engine->setTrackGain(static_cast<int>(trackId), static_cast<float>(gain));
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeToggleTrackMuteStatic(JNIEnv *env, jclass /* cls */, jint trackId) {
    if (!engine) return JNI_FALSE;
    bool ok = engine->toggleTrackMute(static_cast<int>(trackId));
    return ok ? JNI_TRUE : JNI_FALSE;
}

// Clip / transport
extern "C" JNIEXPORT jint JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeCreateClipFromSampleStatic(JNIEnv *env, jclass /* cls */, jint sampleId, jint trackId, jlong startFrame) {
    if (!engine) return -1;
    int id = engine->createClipFromSample(static_cast<int>(sampleId), static_cast<int>(trackId), static_cast<uint64_t>(startFrame));
    return id;
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeSetTransportPlayStatic(JNIEnv *env, jclass /* cls */, jboolean play) {
    if (!engine) return;
    engine->setTransportPlay(play == JNI_TRUE);
}

extern "C" JNIEXPORT void JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeSeekTransportStatic(JNIEnv *env, jclass /* cls */, jlong frame) {
    if (!engine) return;
    engine->seekTransport(static_cast<uint64_t>(frame));
}

// Thumbnail JNI: returns float[] of length width with normalized peak values
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeGetSampleThumbnailStatic(JNIEnv *env, jclass /* cls */, jint sampleId, jint width) {
    jfloatArray result = nullptr;
    if (!engine) return result;
    std::vector<float> thumb = engine->getSampleThumbnail(static_cast<int>(sampleId), static_cast<int>(width));
    if (thumb.empty()) return result;
    result = env->NewFloatArray(static_cast<jsize>(thumb.size()));
    env->SetFloatArrayRegion(result, 0, static_cast<jsize>(thumb.size()), thumb.data());
    return result;
}
