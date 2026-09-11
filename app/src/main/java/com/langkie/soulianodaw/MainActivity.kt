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
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import android.content.pm.PackageManager
import java.io.File

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
    val context = LocalContext.current
    val activity = context as Activity

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.Center,
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
                }
            }) {
                Text(if (!recording) "Record" else "Stop Rec")
            }
        }

        Spacer(modifier = Modifier.height(24.dp))

        Text("Timeline placeholder (implement editor / tracks UI)")
    }
}
