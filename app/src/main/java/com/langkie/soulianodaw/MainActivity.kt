package com.langkie.soulianodaw

import android.Manifest
import android.os.Bundle
import android.content.pm.PackageManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material.Button
import androidx.compose.material.Text
import androidx.compose.material.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        System.loadLibrary("souliano_native")
        nativeInit()

        setContent {
            MaterialTheme {
                MainUI()
            }
        }
    }

    external fun nativeInit()
    external fun nativeStart()
    external fun nativeStop()
    external fun nativeToggleRecord(enable: Boolean)
}

@Composable
fun MainUI() {
    val context = LocalContext.current

    var playing by remember { mutableStateOf(false) }
    var recording by remember { mutableStateOf(false) }

    var recordPermissionGranted by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.RECORD_AUDIO
            ) == PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted: Boolean ->
        recordPermissionGranted = granted
        if (granted) {
            // If permission granted as part of a request to start recording, start recording
            recording = true
            nativeToggleRecordStatic(true)
        }
    }

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
                    // call native start
                    nativeStartStatic()
                } else {
                    playing = false
                    nativeStopStatic()
                }
            }) {
                Text(if (!playing) "Play" else "Stop")
            }

            Button(onClick = {
                if (!recording) {
                    if (recordPermissionGranted) {
                        recording = true
                        nativeToggleRecordStatic(true)
                    } else {
                        // Request permission; if granted the launcher callback will start recording
                        permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                    }
                } else {
                    recording = false
                    nativeToggleRecordStatic(false)
                }
            }) {
                Text(if (!recording) "Record" else "Stop Rec")
            }
        }

        Spacer(modifier = Modifier.height(24.dp))

        if (!recordPermissionGranted) {
            Text("Microphone permission required to record. Tap Record to request.")
        }

        Spacer(modifier = Modifier.height(12.dp))

        Text("Timeline placeholder (implement editor / tracks UI)")
    }
}

// JNI bridge via static functions so Compose lambdas can call them without an activity reference
external fun nativeInitStatic()
external fun nativeStartStatic()
external fun nativeStopStatic()
external fun nativeToggleRecordStatic(enable: Boolean)
