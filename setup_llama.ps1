# Setup script to download llama.cpp source for Android build
# Run this script before building the Android app

$LLAMA_CPP_DIR = "app\src\main\cpp\llama.cpp"

Write-Host "Setting up llama.cpp for Android build..." -ForegroundColor Green

# Check if directory exists
if (Test-Path $LLAMA_CPP_DIR) {
    Write-Host "llama.cpp directory already exists. Removing old version..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $LLAMA_CPP_DIR
}

# Clone llama.cpp repository
Write-Host "Cloning llama.cpp from GitHub..." -ForegroundColor Cyan
git clone --depth 1 --branch master https://github.com/ggerganov/llama.cpp.git $LLAMA_CPP_DIR

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to clone llama.cpp repository" -ForegroundColor Red
    Write-Host "Make sure you have git installed and internet connection" -ForegroundColor Red
    exit 1
}

# Verify required files exist
$required_files = @(
    "$LLAMA_CPP_DIR\ggml.c",
    "$LLAMA_CPP_DIR\ggml-alloc.c",
    "$LLAMA_CPP_DIR\ggml-backend.c",
    "$LLAMA_CPP_DIR\ggml-quants.c",
    "$LLAMA_CPP_DIR\llama.cpp",
    "$LLAMA_CPP_DIR\include\llama.h"
)

$all_files_exist = $true
foreach ($file in $required_files) {
    if (-not (Test-Path $file)) {
        Write-Host "ERROR: Required file missing: $file" -ForegroundColor Red
        $all_files_exist = $false
    }
}

if (-not $all_files_exist) {
    Write-Host "ERROR: Some required llama.cpp files are missing" -ForegroundColor Red
    exit 1
}

Write-Host "`n✓ llama.cpp setup complete!" -ForegroundColor Green
Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Build the Android app: .\gradlew assembleDebug" -ForegroundColor White
Write-Host "2. The native library will be compiled automatically" -ForegroundColor White
Write-Host "3. Install APK on your device" -ForegroundColor White
Write-Host "4. Download a .gguf model (e.g., Llama-3.1-8B-Q4_K_M.gguf)" -ForegroundColor White
Write-Host "5. Place the model in your device's Downloads folder" -ForegroundColor White
Write-Host "6. Use the app file picker to select the model" -ForegroundColor White

