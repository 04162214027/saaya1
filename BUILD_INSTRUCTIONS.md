# Build Instructions for Saaya AI

## Prerequisites

1. **Install Android Studio**
   - Download from: https://developer.android.com/studio
   - Install with Android SDK

2. **Install JDK 17**
   - Download from: https://adoptium.net/
   - Set JAVA_HOME environment variable

## Building the APK

### Method 1: Using GitHub Actions (Recommended)

1. Push code to GitHub:
   ```bash
   # Run the push_to_github.bat script
   push_to_github.bat
   ```

2. GitHub Actions will automatically:
   - Build the debug APK
   - Create a release
   - Upload the APK as an artifact

3. Download the APK from:
   - **Actions** tab → Latest workflow run → Artifacts
   - **Releases** tab → Latest release

### Method 2: Using Android Studio

1. Open Android Studio
2. Click "Open an existing project"
3. Navigate to the SAAYA folder
4. Wait for Gradle sync to complete
5. Click **Build** → **Build Bundle(s) / APK(s)** → **Build APK(s)**
6. APK location: `app/build/outputs/apk/debug/app-debug.apk`

### Method 3: Using Command Line

```bash
# Navigate to project directory
cd SAAYA

# Windows
gradlew.bat assembleDebug

# Linux/Mac
./gradlew assembleDebug
```

APK will be at: `app/build/outputs/apk/debug/app-debug.apk`

## Installing on Android Device

### Enable Developer Mode
1. Go to **Settings** → **About Phone**
2. Tap **Build Number** 7 times
3. Developer options enabled!

### Install APK
1. Transfer APK to device
2. Open file manager
3. Tap the APK file
4. Allow "Install from unknown sources"
5. Tap **Install**

## First Run Setup

1. Launch Saaya AI
2. Grant storage permissions when prompted
3. Select your `.gguf` model file (Llama 3.1 8B)
4. Wait for model to load (1-2 minutes)
5. Start chatting!

## Downloading Llama 3.1 8B Model

### Option 1: Hugging Face
```
https://huggingface.co/models?search=llama-3.1-8B-gguf
```

### Option 2: Direct Downloads
Look for files ending in `.gguf` with Q4 or Q5 quantization:
- `llama-3.1-8B-Q4_K_M.gguf` (~5GB)
- `llama-3.1-8B-Q5_K_M.gguf` (~6GB)

### Recommended Quantization
- **Q4_K_M**: Best balance (5GB)
- **Q5_K_M**: Better quality (6GB)
- **Q8_0**: Highest quality (8GB)

## Troubleshooting

### Build Errors

**Error: SDK not found**
```
File → Settings → Android SDK → Install Android 13.0 (API 33)
```

**Error: Java version mismatch**
```
File → Settings → Build Tools → Gradle → Gradle JDK → Select JDK 17
```

### Runtime Errors

**App crashes on launch**
- Check Android version (minimum 7.0)
- Grant storage permissions
- Clear app data and retry

**Model loading fails**
- Verify .gguf file is not corrupted
- Check available storage (need 6-7GB free)
- Ensure file has .gguf extension

**Out of memory error**
- Close other apps
- Restart device
- Use smaller quantization (Q4 instead of Q8)

## Performance Tips

1. **Use Q4 quantization** for best speed/quality balance
2. **Close background apps** before loading model
3. **Restart device** if experiencing slowdowns
4. **Use airplane mode** for better performance (offline app)

## GitHub Repository Structure

```
SAAYA/
├── .github/workflows/       # CI/CD for auto-building APK
├── app/
│   ├── src/main/
│   │   ├── java/           # Java source code
│   │   ├── res/            # Resources (layouts, strings, etc.)
│   │   └── AndroidManifest.xml
│   └── build.gradle        # App dependencies
├── gradle/                 # Gradle wrapper
├── build.gradle           # Project-level build config
├── settings.gradle        # Project settings
└── push_to_github.bat    # Git push script
```

## Support

For issues or questions:
1. Check the [README.md](README.md)
2. Review error logs in Android Studio
3. Enable "Developer options" → "USB debugging" for detailed logs

---

**Happy Building! 🚀**
