# Saaya AI - Technical Architecture

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         SAAYA AI APP                            │
│                     (com.saaya.ai)                              │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      UI LAYER (Main Thread)                     │
├─────────────────────────────────────────────────────────────────┤
│  MainActivity.java                                              │
│  ├── RecyclerView (Chat Display)                               │
│  │   └── ChatAdapter.java                                       │
│  │       └── ChatMessage.java (Data Model)                      │
│  ├── EditText (User Input)                                      │
│  ├── ImageButton (Send)                                         │
│  ├── ProgressBar (Loading Indicator)                            │
│  └── TextView (Status Display)                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ├─ Handler (Message Passing)
                              │
┌─────────────────────────────────────────────────────────────────┐
│                  BUSINESS LOGIC LAYER                           │
├─────────────────────────────────────────────────────────────────┤
│  ExecutorService (Background Thread Pool)                       │
│  ├── Model Loading Task                                         │
│  ├── Inference Task                                             │
│  └── File Copy Task                                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ├─ JNI Bridge
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    AI MODEL LAYER                               │
├─────────────────────────────────────────────────────────────────┤
│  java-llama.cpp (v0.5.1)                                        │
│  ├── LlamaModel                                                 │
│  │   ├── ModelParameters                                        │
│  │   │   ├── modelFilePath: String                             │
│  │   │   ├── nGpuLayers: int (0 for CPU)                       │
│  │   │   ├── contextSize: int (2048)                            │
│  │   │   └── seed: int                                          │
│  │   └── InferenceParameters                                    │
│  │       ├── temperature: float (0.7)                           │
│  │       ├── topP: float (0.9)                                  │
│  │       ├── topK: int (40)                                     │
│  │       └── maxTokens: int (512)                               │
│  └── Native Libraries (.so files)                               │
│      ├── libllama.so                                            │
│      └── libggml.so                                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ├─ File I/O
                              │
┌─────────────────────────────────────────────────────────────────┐
│                    STORAGE LAYER                                │
├─────────────────────────────────────────────────────────────────┤
│  Internal Storage                                               │
│  └── /data/data/com.saaya.ai/files/models/                     │
│      └── llama-3.1-8B-Q4_K_M.gguf (5GB)                         │
│                                                                  │
│  External Storage                                               │
│  └── /storage/emulated/0/Download/                             │
│      └── [User's original .gguf files]                          │
│                                                                  │
│  SharedPreferences                                              │
│  └── SaayaPrefs                                                 │
│      └── model_path: String                                     │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow

### 1. App Launch Flow
```
User Launches App
    ↓
MainActivity.onCreate()
    ↓
Check SharedPreferences for saved model path
    ↓
┌─────────────┬──────────────┐
│ Path Exists │ Path Missing │
└─────────────┴──────────────┘
      ↓              ↓
Load Model    Request Permissions
      ↓              ↓
   Ready      Show File Picker
                     ↓
              User Selects .gguf
                     ↓
               Copy to Internal
                     ↓
              Save Path to Prefs
                     ↓
               Load Model
                     ↓
                  Ready
```

### 2. Message Send Flow
```
User Types Message → Clicks Send
         ↓
Add User Message to RecyclerView
         ↓
Clear Input Field
         ↓
Create Placeholder AI Message
         ↓
Submit to ExecutorService (Background Thread)
         ↓
LlamaModel.generate(userMessage, params)
         ↓
For Each Token Generated:
    ↓
Append to Response StringBuilder
    ↓
Handler.post → Update UI (Main Thread)
    ↓
Update AI Message in RecyclerView
    ↓
Scroll to Bottom
         ↓
Generation Complete
         ↓
Enable Input Field
```

### 3. Permission Request Flow
```
Android 11+:
    ↓
Check Environment.isExternalStorageManager()
    ↓
If False → Show Dialog
    ↓
User Grants → ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION
    ↓
onActivityResult → Open File Picker

Android 10 and Below:
    ↓
Check READ_EXTERNAL_STORAGE Permission
    ↓
If Not Granted → requestPermissions()
    ↓
onRequestPermissionsResult → Open File Picker
```

## Memory Architecture

```
┌────────────────────────────────────────────┐
│          Device Memory (16GB)              │
├────────────────────────────────────────────┤
│  Physical RAM: 8GB                         │
│  Virtual RAM:  8GB (ZRAM compression)      │
└────────────────────────────────────────────┘
                 │
    ┌────────────┴────────────┐
    │                         │
┌───▼────┐              ┌─────▼────┐
│ System │              │   App    │
│  4GB   │              │  Heap    │
└────────┘              └──────────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
         ┌────▼───┐    ┌─────▼────┐   ┌────▼────┐
         │UI/View │    │AI Model  │   │ Native  │
         │ 100MB  │    │  5-6GB   │   │  Libs   │
         └────────┘    └──────────┘   └─────────┘
                       (llama.cpp)     (JNI .so)
```

## Threading Model

```
┌─────────────────────────────────────────────────────────┐
│                    MAIN THREAD (UI)                     │
│  - Render Views                                         │
│  - Handle User Input                                    │
│  - Update RecyclerView                                  │
│  - Show/Hide Progress Indicators                        │
└─────────────────────────────────────────────────────────┘
                        │
                        │ Handler Messages
                        ↕
┌─────────────────────────────────────────────────────────┐
│              EXECUTOR SERVICE THREAD POOL                │
│  (Single Thread Executor for Sequential Processing)     │
│                                                          │
│  Thread 1: Model Loading                                │
│    - Read .gguf file from disk                          │
│    - Initialize llama.cpp model                         │
│    - Allocate memory for weights                        │
│    - Send completion to Main Thread                     │
│                                                          │
│  Thread 1 (Reused): Inference                           │
│    - Tokenize user input                                │
│    - Run inference loop                                 │
│    - Generate tokens one by one                         │
│    - Stream results to Main Thread                      │
│                                                          │
│  Thread 1 (Reused): File Operations                     │
│    - Copy .gguf from external to internal storage       │
│    - Monitor progress                                   │
│    - Report status to Main Thread                       │
└─────────────────────────────────────────────────────────┘
```

## Component Interaction Matrix

| Component | Interacts With | Purpose |
|-----------|---------------|---------|
| **MainActivity** | ChatAdapter | Manages chat messages |
| **MainActivity** | LlamaModel | AI inference calls |
| **MainActivity** | SharedPreferences | Persist model path |
| **MainActivity** | ExecutorService | Background tasks |
| **MainActivity** | Handler | Thread communication |
| **ChatAdapter** | RecyclerView | Display messages |
| **ChatAdapter** | ChatMessage | Data binding |
| **LlamaModel** | Native (.so) | JNI calls to C++ |
| **ExecutorService** | Handler | Post results to UI |

## File System Structure

```
Device Storage
│
├── /storage/emulated/0/          (External Storage)
│   └── Download/
│       └── llama-3.1-8B-Q4_K_M.gguf  (User downloads here)
│
└── /data/data/com.saaya.ai/      (App Private Storage)
    ├── shared_prefs/
    │   └── SaayaPrefs.xml
    │       └── <string name="model_path">...</string>
    │
    └── files/
        └── models/
            └── llama-3.1-8B-Q4_K_M.gguf  (Copied here for fast access)
```

## Build & Deployment Pipeline

```
Developer Machine
    ↓
Git Push to GitHub
    ↓
┌─────────────────────────────────────┐
│     GitHub Actions Workflow         │
├─────────────────────────────────────┤
│ 1. Checkout code                    │
│ 2. Setup JDK 17                     │
│ 3. Cache Gradle dependencies        │
│ 4. chmod +x gradlew                 │
│ 5. ./gradlew assembleDebug          │
│ 6. Upload APK as artifact           │
│ 7. Create GitHub Release            │
│ 8. Attach APK to release            │
└─────────────────────────────────────┘
    ↓
APK Available for Download
    ↓
User Installs on Android Device
    ↓
App Ready to Use
```

## Performance Optimization Strategies

### 1. Memory Management
- **Large Heap**: Enabled via `android:largeHeap="true"`
- **Model Caching**: Keep model in memory during app lifecycle
- **Efficient Tokenization**: Reuse tokenizer instance

### 2. Threading
- **Single Executor**: Prevents multiple simultaneous inferences
- **Handler Pattern**: Efficient UI updates from background
- **Streaming Output**: Updates UI as tokens generate

### 3. File I/O
- **Internal Storage Copy**: Faster than reading from external
- **Buffered Streams**: 1MB buffer for large file copying
- **Progress Callbacks**: Don't block on file operations

### 4. UI Responsiveness
- **Background Loading**: All heavy operations off main thread
- **RecyclerView**: Efficient list rendering
- **View Recycling**: Minimal memory for chat history

## Security Considerations

```
┌────────────────────────────────────────┐
│         Permission Boundaries          │
├────────────────────────────────────────┤
│ MANAGE_EXTERNAL_STORAGE                │
│  → Only for reading .gguf files        │
│  → No write access used                │
│                                        │
│ Internal Storage (/data/data/...)     │
│  → App-private, secure                 │
│  → Auto-deleted on uninstall           │
│                                        │
│ No Network Permissions Required        │
│  → 100% offline operation              │
│  → No data sent externally             │
└────────────────────────────────────────┘
```

## Technology Stack

| Layer | Technology |
|-------|-----------|
| **Language** | Java 8 |
| **UI Framework** | Android SDK 34 (Material Design) |
| **AI Engine** | llama.cpp (C++) via JNI |
| **Java Wrapper** | java-llama.cpp v0.5.1 |
| **Build System** | Gradle 8.1 |
| **Min Android** | API 24 (Android 7.0) |
| **CI/CD** | GitHub Actions |
| **Version Control** | Git |

---

**This architecture ensures:**
✅ Smooth 60 FPS UI performance
✅ No main thread blocking
✅ Efficient 5GB model handling
✅ Minimal battery consumption
✅ Complete offline functionality
✅ Scalable for future features

*Architecture designed by Senior Android Java Developer*
*Last updated: February 4, 2026*
