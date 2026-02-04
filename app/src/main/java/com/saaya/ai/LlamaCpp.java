package com.saaya.ai;

import android.util.Log;

/**
 * JNI interface to llama.cpp native library
 * This provides REAL inference using llama.cpp C++ code
 */
public class LlamaCpp {
    private static final String TAG = "LlamaCpp";
    
    static {
        try {
            System.loadLibrary("llama-android");
            Log.i(TAG, "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
            throw e;
        }
    }
    
    /**
     * Initialize llama.cpp backend
     * Must be called before any other operations
     */
    public static native void initBackend();
    
    /**
     * Free llama.cpp backend resources
     * Call when app is closing
     */
    public static native void freeBackend();
    
    /**
     * Load GGUF model from file path
     * 
     * @param modelPath Absolute path to .gguf model file
     * @param nCtx Context size (e.g., 2048)
     * @param nThreads Number of CPU threads to use
     * @return true if model loaded successfully
     */
    public static native boolean loadModel(String modelPath, int nCtx, int nThreads);
    
    /**
     * Generate text completion for given prompt
     * 
     * @param prompt Input text prompt
     * @param maxTokens Maximum number of tokens to generate
     * @return Generated text response
     */
    public static native String generateToken(String prompt, int maxTokens);
    
    /**
     * Unload current model and free memory
     */
    public static native void unloadModel();
    
    /**
     * Get information about loaded model
     * 
     * @return Model info string
     */
    public static native String getModelInfo();
    
    /**
     * Check if a model is currently loaded
     */
    private static boolean isModelLoaded = false;
    
    public static boolean isLoaded() {
        return isModelLoaded;
    }
    
    static void setLoaded(boolean loaded) {
        isModelLoaded = loaded;
    }
}
