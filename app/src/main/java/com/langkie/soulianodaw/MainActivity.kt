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
    val activity = LocalContext.current as? MainActivity

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
            recording = true
            activity?.nativeToggleRecord(true)
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
                    activity?.nativeStart()
                } else {
                    playing = false
                    activity?.nativeStop()
                }
            }) {
                Text(if (!playing) "Play" else "Stop")
            }

            Button(onClick = {
                if (!recording) {
                    if (recordPermissionGranted) {
                        recording = true
                        activity?.nativeToggleRecord(true)
                    } else {
                        permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                    }
                } else {
                    recording = false
                    activity?.nativeToggleRecord(false)
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
