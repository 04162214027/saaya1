# SAAYA - Offline AI Chat App for Android

A fully functional Android app that runs Llama 3.1 8B (or any compatible .gguf model) **completely offline** on your device using llama.cpp native inference.

## Features

✅ **Real AI Inference** - Uses actual llama.cpp C++ library with JNI bindings (NOT a placeholder)  
✅ **Offline Operation** - No internet required after model is loaded  
✅ **GGUF Format Support** - Compatible with any GGUF quantized model  
✅ **File Picker** - Easy model selection from device storage  
✅ **Memory Efficient** - Uses memory mapping for large models (5GB+)  
✅ **Multi-threaded** - Optimizes CPU usage for faster inference  
✅ **Material Design UI** - Clean chat interface with RecyclerView  
✅ **Background Processing** - UI stays responsive during inference  

## Requirements

- **Device**: Android 7.0+ (API 24+), 16GB RAM recommended for 8B models
- **Model**: Any GGUF quantized model (e.g., Llama-3.1-8B-Q4_K_M.gguf ~4.7GB)
- **Storage**: Enough space for model file + app (~5GB total)
- **Build Tools**: 
  - Android Studio Hedgehog or later
  - Android SDK 34
  - Android NDK 25.0 or later
  - CMake 3.22.1+
  - Git

## Setup Instructions

### 1. Clone llama.cpp Source

Before building the app, you need to download the llama.cpp library:

```powershell
# Run the setup script from the project root
.\setup_llama.ps1
```

This script will:
- Clone llama.cpp from GitHub into `app/src/main/cpp/llama.cpp/`
- Verify all required source files are present
- Display next steps

**Manual Alternative:**
```powershell
git clone --depth 1 https://github.com/ggerganov/llama.cpp.git app/src/main/cpp/llama.cpp
```

### 2. Build the APK

#### Option A: Command Line (Recommended)

```powershell
# Debug build
.\gradlew assembleDebug

# Release build
.\gradlew assembleRelease
```

The APK will be generated at:
- Debug: `app/build/outputs/apk/debug/app-debug.apk`
- Release: `app/build/outputs/apk/release/app-release.apk`

#### Option B: Android Studio

1. Open project in Android Studio
2. Let Gradle sync complete
3. Build → Generate Signed Bundle / APK → APK
4. Select debug or release variant

#### Option C: GitHub Actions (Automatic)

The repository includes a GitHub Actions workflow that automatically builds APKs on push:

1. Push code to GitHub
2. GitHub Actions will compile the native library and build the APK
3. Download the artifact from the Actions tab

### 3. Get a GGUF Model

Download a quantized GGUF model. Recommended options:

**Llama 3.1 8B (Recommended for 16GB RAM devices):**
```
Model: Llama-3.1-8B-Instruct-Q4_K_M.gguf
Size: ~4.7 GB
Download: https://huggingface.co/lmstudio-ai/Meta-Llama-3.1-8B-Instruct-GGUF
```

**Smaller alternatives for lower RAM:**
- Llama 3.1 1B Q4_K_M (~900MB)
- Phi-3-mini Q4_K_M (~2.3GB)

Transfer the .gguf file to your Android device's **Downloads** folder.

### 4. Install and Run

1. Install APK on your Android device
2. Grant storage permissions when prompted
3. Use the file picker to select your .gguf model file
4. Wait for model loading (may take 30-60 seconds for 5GB models)
5. Start chatting!

## Technical Architecture

### Native Components (C++)

- **llama-android.cpp** - JNI wrapper bridging Java and llama.cpp
- **llama.cpp library** - Core inference engine (ggerganov/llama.cpp)
- **CMake build system** - Compiles native code for Android ABIs

### Java Components

- **MainActivity.java** - Chat UI, file picker, threading orchestration
- **LlamaCpp.java** - Java interface for native methods
- **ChatAdapter.java** - RecyclerView adapter for messages
- **ChatMessage.java** - Message data model

### Key Technologies

- **JNI (Java Native Interface)** - Calls C++ code from Java
- **llama.cpp** - Fast LLM inference in C/C++
- **Memory Mapping** - Loads 5GB models without copying to RAM
- **Threading** - Background ExecutorService for non-blocking inference
- **Material Design** - Modern Android UI components

## Code Overview

### Native Inference Flow

```java
// 1. Initialize backend (one-time setup)
LlamaCpp.initBackend();

// 2. Load model with context size and threads
boolean success = LlamaCpp.loadModel(
    "/storage/emulated/0/Download/model.gguf",
    2048,  // context size
    4      // CPU threads
);

// 3. Generate response
String response = LlamaCpp.generateToken(
    "Hello, how are you?",
    512    // max tokens
);

// 4. Cleanup when done
LlamaCpp.unloadModel();
LlamaCpp.freeBackend();
```

### JNI Method Signatures (llama-android.cpp)

```cpp
JNIEXPORT void JNICALL Java_com_saaya_ai_LlamaCpp_initBackend
JNIEXPORT jboolean JNICALL Java_com_saaya_ai_LlamaCpp_loadModel
JNIEXPORT jstring JNICALL Java_com_saaya_ai_LlamaCpp_generateToken
JNIEXPORT void JNICALL Java_com_saaya_ai_LlamaCpp_unloadModel
JNIEXPORT jstring JNICALL Java_com_saaya_ai_LlamaCpp_getModelInfo
```

### CMake Build Configuration

The `CMakeLists.txt` compiles these llama.cpp source files:
- `ggml.c` - Core tensor operations
- `ggml-alloc.c` - Memory allocation
- `ggml-backend.c` - Backend abstraction
- `ggml-quants.c` - Quantization support
- `llama.cpp` - Main LLM API

Optimization flags: `-O3 -DNDEBUG -ffast-math`

## Performance Notes

### Model Loading
- **5GB model on 16GB RAM**: 30-60 seconds
- Uses memory mapping to avoid copying entire model to RAM
- Loading time depends on storage speed (SSD faster than SD card)

### Inference Speed
- **8B Q4 model**: ~1-3 tokens/second on modern ARM CPUs
- Utilizes multiple CPU threads (detected automatically)
- Context size affects memory and speed

### Memory Usage
- App has `largeHeap` enabled in manifest
- Model loaded with `use_mmap=true` (efficient for large files)
- Context size of 2048 tokens uses ~512MB additional RAM

## Troubleshooting

### Build Errors

**"Cannot find llama.cpp sources"**
```powershell
# Run setup script
.\setup_llama.ps1
```

**"NDK not configured"**
- Open Android Studio → Tools → SDK Manager → SDK Tools
- Check "NDK (Side by side)" and install

**"CMake not found"**
- In SDK Manager, check "CMake" and install version 3.22.1+

### Runtime Errors

**"Failed to load native library"**
- Rebuild app to ensure native library is compiled
- Check if APK contains `lib/arm64-v8a/libllama-android.so`

**"Model loading failed"**
- Verify .gguf file is valid and not corrupted
- Check device has enough free RAM (16GB for 8B models)
- Ensure storage permissions are granted

**"App crashes on model load"**
- Try a smaller quantized model (Q4 instead of Q5/Q6)
- Close background apps to free memory
- Enable developer options → "Don't keep activities" = OFF

## File Structure

```
SAAYA/
├── app/
│   ├── src/main/
│   │   ├── cpp/
│   │   │   ├── llama.cpp/          # (Downloaded via setup script)
│   │   │   ├── llama-android.cpp   # JNI wrapper
│   │   │   └── CMakeLists.txt      # Native build config
│   │   ├── java/com/saaya/ai/
│   │   │   ├── MainActivity.java   # Main activity
│   │   │   ├── LlamaCpp.java       # Native method interface
│   │   │   ├── ChatAdapter.java    # RecyclerView adapter
│   │   │   └── ChatMessage.java    # Message model
│   │   ├── res/layout/
│   │   │   ├── activity_main.xml   # Main chat UI
│   │   │   └── item_message.xml    # Chat bubble layout
│   │   └── AndroidManifest.xml     # Permissions & config
│   └── build.gradle                # App-level build config
├── build.gradle                    # Project-level build config
├── settings.gradle                 # Gradle settings
├── setup_llama.ps1                 # Setup script
└── README.md                       # This file
```

## Model Recommendations

| Model | Size | RAM Required | Speed | Quality |
|-------|------|--------------|-------|---------|
| Llama 3.1 1B Q4_K_M | 900MB | 4GB+ | Fast | Good |
| Phi-3-mini Q4_K_M | 2.3GB | 8GB+ | Medium | Very Good |
| Llama 3.1 8B Q4_K_M | 4.7GB | 16GB+ | Slower | Excellent |

## GitHub Actions Workflow

The `.github/workflows/android-build.yml` automatically:
1. Sets up JDK 17 and Android SDK
2. Installs NDK and CMake
3. Clones llama.cpp sources
4. Builds debug and release APKs
5. Uploads APKs as artifacts

## License

This app integrates:
- **llama.cpp** - MIT License (ggerganov/llama.cpp)
- **Android Material Components** - Apache 2.0

## Credits

- **llama.cpp** by Georgi Gerganov and contributors
- **Meta Llama** models by Meta AI
- **Android Development** using AndroidX libraries

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on real Android device
5. Submit pull request

## Support

For issues related to:
- **App functionality**: Open an issue in this repository
- **llama.cpp library**: Check [llama.cpp GitHub](https://github.com/ggerganov/llama.cpp)
- **Model downloads**: Visit [Hugging Face](https://huggingface.co/models)

---

**Built with ❤️ for offline AI on Android**
