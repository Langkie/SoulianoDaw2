package com.langkie.soulianodaw

import android.app.Activity
import android.os.Bundle
import android.Manifest
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material.Button
import androidx.compose.material.Text
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Slider
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import android.content.pm.PackageManager
import java.io.File
import android.widget.Toast

class MainActivity : ComponentActivity() {
    companion object {
        // used to pass a pending path when requesting permission
        var pendingRecordingPath: String? = null
        const val REQUEST_RECORD_AUDIO = 101

        // static JNI bindings used by Compose callbacks
        external fun nativeInitStatic()
        external fun nativeStartStatic()
        external fun nativeStopStatic()
        external fun nativeToggleRecordStatic(enable: Boolean)
        external fun nativeStartRecordingStatic(path: String): Boolean
        external fun nativeStopRecordingStatic()
        external fun nativeLoadSampleStatic(path: String): Int
        external fun nativeTriggerSampleStatic(sampleId: Int): Boolean
        external fun nativeCreateTrackWithSampleStatic(path: String): Int
        external fun nativeSetTrackGainStatic(trackId: Int, gain: Float): Boolean
        external fun nativeToggleTrackMuteStatic(trackId: Int): Boolean
        external fun nativeExportMixdownStatic(path: String): Boolean
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("souliano_native")
        nativeInitStatic()

        setContent {
            MaterialTheme {
                MainUI()
            }
        }
    }

    // Legacy member bindings (not used by Compose directly but kept for completeness)
    external fun nativeInit()
    external fun nativeStart()
    external fun nativeStop()
    external fun nativeToggleRecord(enable: Boolean)
    external fun nativeStartRecording(path: String): Boolean
    external fun nativeStopRecording()

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_RECORD_AUDIO) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // start recording if we had a pending path
                pendingRecordingPath?.let { path ->
                    val ok = nativeStartRecordingStatic(path)
                    Log.i("Souliano", "Started recording after permission: $ok")
                }
            } else {
                Log.w("Souliano", "Record permission denied")
            }
            pendingRecordingPath = null
        }
    }
}

@Composable
fun MainUI() {
    var playing by remember { mutableStateOf(false) }
    var recording by remember { mutableStateOf(false) }
    var lastTrackId by remember { mutableStateOf(-1) }
    var tracksCount by remember { mutableStateOf(0) }
    val context = LocalContext.current
    val activity = context as Activity

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.Top,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Button(onClick = {
                if (!playing) {
                    playing = true
                    MainActivity.nativeStartStatic()
                } else {
                    playing = false
                    MainActivity.nativeStopStatic()
                }
            }) {
                Text(if (!playing) "Play" else "Stop")
            }

            Button(onClick = {
                if (!recording) {
                    // start recording: ensure permission
                    val hasPermission = ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
                    val recordingsDir = File(activity.getExternalFilesDir(null), "recordings")
                    recordingsDir.mkdirs()
                    val outPath = File(recordingsDir, "rec_${System.currentTimeMillis()}.wav").absolutePath

                    if (!hasPermission) {
                        // request and store pending path
                        MainActivity.pendingRecordingPath = outPath
                        ActivityCompat.requestPermissions(activity, arrayOf(Manifest.permission.RECORD_AUDIO), MainActivity.REQUEST_RECORD_AUDIO)
                    } else {
                        val started = MainActivity.nativeStartRecordingStatic(outPath)
                        if (started) recording = true
                    }
                } else {
                    // stop recording
                    MainActivity.nativeStopRecordingStatic()
                    recording = false
                    // create a new track from the newest recording
                    val recordingsDir = File(activity.getExternalFilesDir(null), "recordings")
                    val files = recordingsDir.listFiles()?.sortedByDescending { it.lastModified() }
                    if (files != null && files.isNotEmpty()) {
                        val newest = files[0]
                        val trackId = MainActivity.nativeCreateTrackWithSampleStatic(newest.absolutePath)
                        if (trackId >= 0) {
                            lastTrackId = trackId
                            tracksCount = trackId + 1
                        }
                    }
                }
            }) {
                Text(if (!recording) "Record" else "Stop Rec")
            }

            Spacer(modifier = Modifier.width(8.dp))

            Button(onClick = {
                // Export mixdown of all tracks
                val outDir = File(activity.getExternalFilesDir(null), "exports")
                outDir.mkdirs()
                val outPath = File(outDir, "mixdown_${System.currentTimeMillis()}.wav").absolutePath
                val ok = MainActivity.nativeExportMixdownStatic(outPath)
                if (ok) {
                    Toast.makeText(context, "Exported mixdown: $outPath", Toast.LENGTH_LONG).show()
                } else {
                    Toast.makeText(context, "Mixdown failed", Toast.LENGTH_SHORT).show()
                }
            }) {
                Text("Export Mixdown")
            }
        }

        Spacer(modifier = Modifier.height(12.dp))

        Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Button(onClick = {
                // load last recording as a track without recording
                val recordingsDir = File(activity.getExternalFilesDir(null), "recordings")
                val files = recordingsDir.listFiles()?.sortedByDescending { it.lastModified() }
                if (files != null && files.isNotEmpty()) {
                    val newest = files[0]
                    val trackId = MainActivity.nativeCreateTrackWithSampleStatic(newest.absolutePath)
                    if (trackId >= 0) {
                        lastTrackId = trackId
                        tracksCount = trackId + 1
                    }
                }
            }) {
                Text("Load Last Rec as Track")
            }

            Spacer(modifier = Modifier.width(12.dp))

            Button(onClick = {
                // trigger last track if exists
                if (lastTrackId >= 0) {
                    // trigger the underlying sample
                    // We don't have a direct track-play function; trigger sample by mapping track -> sample
                    MainActivity.nativeTriggerSampleStatic(lastTrackId)
                }
            }) {
                Text("Play Last Track")
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Track list UI
        TrackListUI(tracksCount,
            onSetGain = { tid, gain ->
                MainActivity.nativeSetTrackGainStatic(tid, gain)
            },
            onToggleMute = { tid ->
                MainActivity.nativeToggleTrackMuteStatic(tid)
            },
            onTrigger = { tid ->
                // trigger the sample associated with track id
                MainActivity.nativeTriggerSampleStatic(tid)
            }
        )

        Spacer(modifier = Modifier.height(24.dp))

        Text("Timeline placeholder (implement editor / tracks UI)")
    }
}
