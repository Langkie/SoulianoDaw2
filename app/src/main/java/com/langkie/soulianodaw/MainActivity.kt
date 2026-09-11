package com.langkie.soulianodaw

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material.Button
import androidx.compose.material.Text
import androidx.compose.material.MaterialTheme
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

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
    var playing by remember { mutableStateOf(false) }
    var recording by remember { mutableStateOf(false) }

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
                recording = !recording
                nativeToggleRecordStatic(recording)
            }) {
                Text(if (!recording) "Record" else "Stop Rec")
            }
        }

        Spacer(modifier = Modifier.height(24.dp))

        Text("Timeline placeholder (implement editor / tracks UI)")
    }
}

// JNI bridge via static functions so Compose lambdas can call them without an activity reference
external fun nativeInitStatic()
external fun nativeStartStatic()
external fun nativeStopStatic()
external fun nativeToggleRecordStatic(enable: Boolean)
