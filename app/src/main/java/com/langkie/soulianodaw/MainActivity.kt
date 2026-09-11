package com.langkie.soulianodaw

import android.Manifest
import android.app.Activity
import android.content.ContentValues
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.provider.MediaStore
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.text.SimpleDateFormat
import java.util.*

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
    val activity = context as Activity

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

    var showSettingsDialog by remember { mutableStateOf(false) }

    val permissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted: Boolean ->
        recordPermissionGranted = granted
        if (granted) {
            // Start recording immediately; Compose state toggled by caller
            recording = true
            startRecordingWithPath(context)
            Toast.makeText(context, "Recording started", Toast.LENGTH_SHORT).show()
        } else {
            // If user denied and "Don't ask again" selected, show settings dialog
            val shouldShow = ActivityCompat.shouldShowRequestPermissionRationale(activity, Manifest.permission.RECORD_AUDIO)
            if (!shouldShow) {
                showSettingsDialog = true
            }
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
                        startRecordingWithPath(context)
                        Toast.makeText(context, "Recording started", Toast.LENGTH_SHORT).show()
                    } else {
                        // Request permission; if granted the launcher callback will start recording
                        permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                    }
                } else {
                    recording = false
                    nativeStopRecordingStatic()
                    Toast.makeText(context, "Recording stopped", Toast.LENGTH_SHORT).show()

                    // Export to MediaStore in background
                    val exportedFile = getLatestRecordingFile(context)
                    if (exportedFile != null) {
                        Thread {
                            val success = exportToMediaStore(context, exportedFile)
                            (context as Activity).runOnUiThread {
                                if (success) {
                                    Toast.makeText(context, "Saved to library", Toast.LENGTH_SHORT).show()
                                } else {
                                    Toast.makeText(context, "Export failed", Toast.LENGTH_SHORT).show()
                                }
                            }
                        }.start()
                    }
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

    if (showSettingsDialog) {
        AlertDialog(
            onDismissRequest = { showSettingsDialog = false },
            title = { Text("Microphone permission required") },
            text = { Text("The app needs microphone permission to record audio. Please enable it in app settings.") },
            confirmButton = {
                TextButton(onClick = {
                    showSettingsDialog = false
                    // Open app settings
                    val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                    val uri: Uri = Uri.fromParts("package", context.packageName, null)
                    intent.data = uri
                    context.startActivity(intent)
                }) {
                    Text("Open settings")
                }
            },
            dismissButton = {
                TextButton(onClick = { showSettingsDialog = false }) { Text("Cancel") }
            }
        )
    }
}

// Utility: build file path and call native start recording
fun startRecordingWithPath(context: Context) {
    val dir = context.getExternalFilesDir(Environment.DIRECTORY_MUSIC)
    if (dir == null) return
    val sdf = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US)
    val name = "souliano_rec_${sdf.format(Date())}.wav"
    val file = File(dir, name)
    val started = nativeStartRecordingStatic(file.absolutePath)
    // started is boolean returned by native; we don't handle false specially here
}

// Utility: find most recent recording in app Music directory (simple heuristic)
fun getLatestRecordingFile(context: Context): File? {
    val dir = context.getExternalFilesDir(Environment.DIRECTORY_MUSIC) ?: return null
    val files = dir.listFiles { f -> f.extension.equals("wav", ignoreCase = true) }
    if (files == null || files.isEmpty()) return null
    return files.maxByOrNull { it.lastModified() }
}

fun exportToMediaStore(context: Context, file: File): Boolean {
    try {
        val values = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, file.name)
            put(MediaStore.MediaColumns.MIME_TYPE, "audio/wav")
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_MUSIC + "/SoulianoDAW")
            }
        }

        val collection = MediaStore.Audio.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
        val resolver = context.contentResolver
        val uri = resolver.insert(collection, values)
        if (uri != null) {
            resolver.openOutputStream(uri).use { out ->
                FileInputStream(file).use { input ->
                    input.copyTo(out!!)
                }
            }
            return true
        }
    } catch (e: IOException) {
        e.printStackTrace()
    }
    return false
}

// JNI bridge via static functions so Compose lambdas can call them without an activity reference
external fun nativeInitStatic()
external fun nativeStartStatic()
external fun nativeStopStatic()
external fun nativeToggleRecordStatic(enable: Boolean)
external fun nativeStartRecordingStatic(path: String): Boolean
external fun nativeStopRecordingStatic()
