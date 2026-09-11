// Mixdown JNI wrapper
extern "C" JNIEXPORT jboolean JNICALL
Java_com_langkie_soulianodaw_MainActivity_nativeExportMixdownStatic(JNIEnv *env, jclass /* cls */, jstring jpath) {
    if (!engine) return JNI_FALSE;
    const char* path = env->GetStringUTFChars(jpath, nullptr);
    bool ok = engine->exportMixdown(std::string(path));
    env->ReleaseStringUTFChars(jpath, path);
    return ok ? JNI_TRUE : JNI_FALSE;
}
