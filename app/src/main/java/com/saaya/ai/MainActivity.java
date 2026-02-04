package com.saaya.ai;

import android.Manifest;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.saaya.ai.LlamaModel.ModelParameters;
import com.saaya.ai.LlamaModel.InferenceParameters;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "SaayaAI";
    private static final String PREFS_NAME = "SaayaPrefs";
    private static final String KEY_MODEL_PATH = "model_path";
    private static final int PERMISSION_REQUEST_CODE = 100;

    // UI Components
    private RecyclerView chatRecyclerView;
    private EditText inputEditText;
    private ImageButton sendButton;
    private ProgressBar progressBar;
    private TextView statusText;
    private ChatAdapter chatAdapter;

    // AI Model
    private LlamaModel model;
    private String modelPath;
    private boolean isModelLoaded = false;
    private ExecutorService executorService;
    private Handler mainHandler;

    // File picker
    private ActivityResultLauncher<Intent> filePickerLauncher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initializeViews();
        initializeExecutor();
        setupFilePickerLauncher();
        
        // Check if model path is already saved
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        modelPath = prefs.getString(KEY_MODEL_PATH, null);

        if (modelPath == null || !new File(modelPath).exists()) {
            // First launch or model deleted - request permissions and show file picker
            checkAndRequestPermissions();
        } else {
            // Model path exists - load it
            loadModel(modelPath);
        }
    }

    private void initializeViews() {
        chatRecyclerView = findViewById(R.id.chatRecyclerView);
        inputEditText = findViewById(R.id.inputEditText);
        sendButton = findViewById(R.id.sendButton);
        progressBar = findViewById(R.id.progressBar);
        statusText = findViewById(R.id.statusText);

        // Setup RecyclerView
        chatAdapter = new ChatAdapter();
        LinearLayoutManager layoutManager = new LinearLayoutManager(this);
        layoutManager.setStackFromEnd(true);
        chatRecyclerView.setLayoutManager(layoutManager);
        chatRecyclerView.setAdapter(chatAdapter);

        // Setup send button
        sendButton.setOnClickListener(v -> sendMessage());
        
        // Add button to change model
        findViewById(R.id.changeModelButton).setOnClickListener(v -> {
            new AlertDialog.Builder(this)
                .setTitle("Change Model")
                .setMessage("Do you want to select a different model file?")
                .setPositiveButton("Yes", (dialog, which) -> openFilePicker())
                .setNegativeButton("Cancel", null)
                .show();
        });
    }

    private void initializeExecutor() {
        executorService = Executors.newSingleThreadExecutor();
        mainHandler = new Handler(Looper.getMainLooper());
    }

    private void setupFilePickerLauncher() {
        filePickerLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                    Uri uri = result.getData().getData();
                    if (uri != null) {
                        handleSelectedFile(uri);
                    }
                } else {
                    if (modelPath == null) {
                        Toast.makeText(this, "Model file is required to use the app", 
                                     Toast.LENGTH_LONG).show();
                        finish();
                    }
                }
            }
        );
    }

    private void checkAndRequestPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // Android 11 and above - use MANAGE_EXTERNAL_STORAGE
            if (!Environment.isExternalStorageManager()) {
                new AlertDialog.Builder(this)
                    .setTitle("Storage Permission Required")
                    .setMessage("This app needs access to storage to load the AI model file (5GB .gguf file)")
                    .setPositiveButton("Grant Permission", (dialog, which) -> {
                        Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                        Uri uri = Uri.fromParts("package", getPackageName(), null);
                        intent.setData(uri);
                        startActivityForResult(intent, PERMISSION_REQUEST_CODE);
                    })
                    .setNegativeButton("Cancel", (dialog, which) -> finish())
                    .setCancelable(false)
                    .show();
            } else {
                openFilePicker();
            }
        } else {
            // Android 10 and below
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        PERMISSION_REQUEST_CODE);
            } else {
                openFilePicker();
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                if (Environment.isExternalStorageManager()) {
                    openFilePicker();
                } else {
                    Toast.makeText(this, "Permission denied. Cannot proceed.", 
                                 Toast.LENGTH_LONG).show();
                    finish();
                }
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                          @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openFilePicker();
            } else {
                Toast.makeText(this, "Permission denied. Cannot proceed.", 
                             Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }

    private void openFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        String[] mimeTypes = {"application/octet-stream", "application/x-gguf", "*/*"};
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        intent.putExtra(Intent.EXTRA_TITLE, "Select .gguf Model File");
        
        filePickerLauncher.launch(intent);
    }

    private void handleSelectedFile(Uri uri) {
        showStatus("Copying model file...");
        
        executorService.execute(() -> {
            try {
                // Get the original filename
                String fileName = getFileName(uri);
                if (fileName == null || !fileName.endsWith(".gguf")) {
                    mainHandler.post(() -> {
                        Toast.makeText(this, "Please select a .gguf file", 
                                     Toast.LENGTH_SHORT).show();
                        if (modelPath == null) {
                            openFilePicker();
                        }
                    });
                    return;
                }

                // Copy file to app's internal storage
                File internalDir = new File(getFilesDir(), "models");
                if (!internalDir.exists()) {
                    internalDir.mkdirs();
                }
                
                File destFile = new File(internalDir, fileName);
                
                // If file already exists and is the same, skip copying
                if (destFile.exists() && destFile.length() > 0) {
                    modelPath = destFile.getAbsolutePath();
                    saveModelPath(modelPath);
                    mainHandler.post(() -> loadModel(modelPath));
                    return;
                }

                // Copy the file
                InputStream inputStream = getContentResolver().openInputStream(uri);
                FileOutputStream outputStream = new FileOutputStream(destFile);
                
                byte[] buffer = new byte[1024 * 1024]; // 1MB buffer
                int length;
                long totalBytes = 0;
                
                while ((length = inputStream.read(buffer)) > 0) {
                    outputStream.write(buffer, 0, length);
                    totalBytes += length;
                    final long finalTotalBytes = totalBytes;
                    
                    // Update progress every 100MB
                    if (totalBytes % (100 * 1024 * 1024) == 0) {
                        mainHandler.post(() -> 
                            showStatus(String.format("Copied %.1f GB...", 
                                     finalTotalBytes / (1024.0 * 1024.0 * 1024.0)))
                        );
                    }
                }
                
                inputStream.close();
                outputStream.close();

                modelPath = destFile.getAbsolutePath();
                saveModelPath(modelPath);

                mainHandler.post(() -> {
                    Toast.makeText(this, "Model file copied successfully", 
                                 Toast.LENGTH_SHORT).show();
                    loadModel(modelPath);
                });

            } catch (Exception e) {
                Log.e(TAG, "Error copying file", e);
                mainHandler.post(() -> {
                    Toast.makeText(this, "Error copying file: " + e.getMessage(), 
                                 Toast.LENGTH_LONG).show();
                    if (modelPath == null) {
                        finish();
                    }
                });
            }
        });
    }

    private String getFileName(Uri uri) {
        String result = null;
        if (uri.getScheme().equals("content")) {
            android.database.Cursor cursor = getContentResolver().query(uri, null, 
                                                                        null, null, null);
            try {
                if (cursor != null && cursor.moveToFirst()) {
                    int index = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                    if (index >= 0) {
                        result = cursor.getString(index);
                    }
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
        if (result == null) {
            result = uri.getPath();
            int cut = result.lastIndexOf('/');
            if (cut != -1) {
                result = result.substring(cut + 1);
            }
        }
        return result;
    }

    private void saveModelPath(String path) {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        prefs.edit().putString(KEY_MODEL_PATH, path).apply();
    }

    private void loadModel(String path) {
        showStatus("Loading AI model...");
        setInputEnabled(false);

        executorService.execute(() -> {
            try {
                Log.d(TAG, "Loading model from: " + path);
                
                ModelParameters modelParams = new ModelParameters()
                        .setModelFilePath(path)
                        .setNGpuLayers(0) // CPU only, set to 99 for GPU if available
                        .setContextSize(2048)
                        .setSeed(42);

                model = new LlamaModel(modelParams);
                isModelLoaded = true;

                mainHandler.post(() -> {
                    showStatus("Model loaded successfully! Ready to chat.");
                    setInputEnabled(true);
                    Toast.makeText(this, "AI Model loaded - Ready!", Toast.LENGTH_SHORT).show();
                });

            } catch (Exception e) {
                Log.e(TAG, "Error loading model", e);
                mainHandler.post(() -> {
                    showStatus("Error loading model: " + e.getMessage());
                    Toast.makeText(this, "Failed to load model: " + e.getMessage(), 
                                 Toast.LENGTH_LONG).show();
                    
                    // Offer to select a different file
                    new AlertDialog.Builder(this)
                        .setTitle("Model Loading Failed")
                        .setMessage("The model file could not be loaded. Would you like to select a different file?")
                        .setPositiveButton("Yes", (dialog, which) -> {
                            modelPath = null;
                            openFilePicker();
                        })
                        .setNegativeButton("Exit", (dialog, which) -> finish())
                        .setCancelable(false)
                        .show();
                });
            }
        });
    }

    private void sendMessage() {
        String userMessage = inputEditText.getText().toString().trim();
        
        if (TextUtils.isEmpty(userMessage)) {
            Toast.makeText(this, "Please enter a message", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!isModelLoaded) {
            Toast.makeText(this, "Model is not loaded yet", Toast.LENGTH_SHORT).show();
            return;
        }

        // Add user message to chat
        chatAdapter.addMessage(new ChatMessage(userMessage, true));
        chatRecyclerView.scrollToPosition(chatAdapter.getItemCount() - 1);
        
        // Clear input
        inputEditText.setText("");
        setInputEnabled(false);
        showStatus("Thinking...");

        // Add placeholder for AI response
        chatAdapter.addMessage(new ChatMessage("Thinking...", false));
        final int aiMessagePosition = chatAdapter.getItemCount() - 1;

        // Generate response in background
        executorService.execute(() -> {
            try {
                InferenceParameters inferParams = new InferenceParameters()
                        .setTemperature(0.7f)
                        .setTopP(0.9f)
                        .setTopK(40)
                        .setMaxTokens(512);

                StringBuilder fullResponse = new StringBuilder();
                
                for (LlamaModel.Output output : model.generate(userMessage, inferParams)) {
                    fullResponse.append(output);
                    
                    final String currentResponse = fullResponse.toString();
                    mainHandler.post(() -> {
                        chatAdapter.updateLastMessage(currentResponse);
                        chatRecyclerView.scrollToPosition(aiMessagePosition);
                    });
                }

                mainHandler.post(() -> {
                    setInputEnabled(true);
                    showStatus("Ready");
                });

            } catch (Exception e) {
                Log.e(TAG, "Error generating response", e);
                mainHandler.post(() -> {
                    chatAdapter.updateLastMessage("Error: " + e.getMessage());
                    setInputEnabled(true);
                    showStatus("Error occurred");
                    Toast.makeText(this, "Error: " + e.getMessage(), 
                                 Toast.LENGTH_SHORT).show();
                });
            }
        });
    }

    private void showStatus(String status) {
        statusText.setText(status);
        progressBar.setVisibility(status.contains("Loading") || status.contains("Copying") 
                                   ? View.VISIBLE : View.GONE);
    }

    private void setInputEnabled(boolean enabled) {
        inputEditText.setEnabled(enabled);
        sendButton.setEnabled(enabled);
        sendButton.setAlpha(enabled ? 1.0f : 0.5f);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        // Clean up model
        if (model != null) {
            try {
                model.close();
            } catch (Exception e) {
                Log.e(TAG, "Error closing model", e);
            }
        }
        
        // Shutdown executor
        if (executorService != null) {
            executorService.shutdown();
        }
    }
}
