# SoulianoDAW2 — Android prototype

This repository contains a scaffold for a native Android DAW prototype focused on Android phones.

Goals & recommendations
- Target: Android phones (ARM64) — minSdk 24 (Android 7.0) recommended for modern audio APIs.
- Tech stack: Native audio engine in C++ using Oboe (low-latency audio), UI in Kotlin with Jetpack Compose, build with Android Gradle + CMake/NDK.
- Why: Oboe gives best low-latency on Android (wraps AAudio/OpenSL), C++ for DSP performance, Compose for fast UI iteration.

MVP features included in this scaffold (skeleton):
- Low-latency audio engine skeleton (Oboe) with a simple oscillator callback.
- JNI bridge to control transport: init/start/stop from Kotlin UI.
- Kotlin + Jetpack Compose UI with Play / Stop / Record buttons and a timeline placeholder.
- Project Gradle files and CMakeLists that fetch Oboe via FetchContent.
- README with build/run instructions and next steps to implement full features.

Must-have features to implement (roadmap):
1. Multi-track playback (sample-based) and scheduling.
2. Recording from microphone (file I/O + WAV writer).
3. Mixer: per-track volume, pan, mute/solo.
4. Basic effects chain (gain, EQ, reverb) — as internal DSP or plugins.
5. Session save/load (JSON), project browser.
6. MIDI support (Android MIDI API) and mapping to synths.
7. Export mixdown to WAV/MP3.
8. Performance tuning (thread priorities, audio buffer sizing).

Build instructions (high level):
1. Open this folder in Android Studio Arctic Fox or later.
2. Ensure Android NDK and CMake are installed (SDK Manager).
3. Build & run on a real Android device (recommended) — USB debugging enabled.

Security/licensing notes
- Oboe is Apache 2.0; follow its license. This scaffold does not include third-party closed-source SDKs (e.g., Superpowered).

What's next (I can do next)
- Implement sample playback & file-based session save/load.
- Add microphone recording and WAV export.
- Add simple mixer UI with per-track faders.
- Setup CI with GitHub Actions for lint/build.

