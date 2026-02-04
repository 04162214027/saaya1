# Saaya AI - Offline AI Chat App

A fully functional Android app that runs a local Llama 3.1 8B AI model completely offline.

## Features

- 🤖 Runs Llama 3.1 8B Quantized Model (.gguf format)
- 📱 Simple, intuitive chat interface
- 🔒 Completely offline - no internet required
- 💾 Handles large 5GB model files efficiently
- ⚡ Optimized for devices with 16GB RAM
- 🎨 Modern Material Design UI

## Requirements

- Android device with Android 7.0 (API 24) or higher
- Minimum 8GB RAM (16GB recommended)
- 6-7GB free storage space
- Llama 3.1 8B model in .gguf format

## Installation

1. Clone this repository
2. Open in Android Studio
3. Build and install the APK
4. On first launch, grant storage permissions
5. Select your .gguf model file from device storage

## Technical Details

- **Language**: Java
- **AI Library**: java-llama.cpp (JNI bindings)
- **UI**: RecyclerView with Material Design
- **Threading**: ExecutorService for background inference
- **Memory**: largeHeap enabled for 5GB model
- **Permissions**: MANAGE_EXTERNAL_STORAGE for Android 11+

## Project Structure

```
app/
├── src/main/
│   ├── java/com/saaya/ai/
│   │   ├── MainActivity.java      # Main activity with chat logic
│   │   ├── ChatAdapter.java       # RecyclerView adapter
│   │   └── ChatMessage.java       # Message data model
│   ├── res/
│   │   ├── layout/               # XML layouts
│   │   ├── drawable/             # UI resources
│   │   └── values/               # Strings, colors
│   └── AndroidManifest.xml       # App configuration
└── build.gradle                  # Dependencies
```

## Building

```bash
./gradlew assembleDebug
```

The APK will be generated in `app/build/outputs/apk/debug/`

## Usage

1. Launch the app
2. Grant storage permissions when prompted
3. Select your .gguf model file
4. Wait for model to load (may take 1-2 minutes)
5. Start chatting with the AI!

## License

MIT License

## Author

Developed by Senior Android Java Developer
