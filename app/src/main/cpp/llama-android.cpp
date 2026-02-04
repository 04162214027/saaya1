#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include "llama.h"
#include "ggml.h"

#define LOG_TAG "LlamaCpp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Helper to add token to batch
static void llama_batch_add(
    struct llama_batch & batch,
    llama_token id,
    llama_pos pos,
    const std::vector<llama_seq_id> & seq_ids,
    bool logits) {
    batch.token[batch.n_tokens] = id;
    batch.pos[batch.n_tokens] = pos;
    batch.n_seq_id[batch.n_tokens] = seq_ids.size();
    for (size_t i = 0; i < seq_ids.size(); ++i) {
        batch.seq_id[batch.n_tokens][i] = seq_ids[i];
    }
    batch.logits[batch.n_tokens] = logits;
    batch.n_tokens++;
}

// Global context holder
struct LlamaContext {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    llama_sampler* sampler = nullptr;
    int n_ctx = 2048;
    int n_threads = 4;
};

static std::unique_ptr<LlamaContext> g_llama_ctx;

extern "C" {

// Initialize backend
JNIEXPORT void JNICALL
Java_com_saaya_ai_LlamaCpp_initBackend(JNIEnv* env, jclass clazz) {
    LOGI("Initializing llama.cpp backend");
    llama_backend_init();
    llama_numa_init(GGML_NUMA_STRATEGY_DISABLED);
}

// Free backend
JNIEXPORT void JNICALL
Java_com_saaya_ai_LlamaCpp_freeBackend(JNIEnv* env, jclass clazz) {
    LOGI("Freeing llama.cpp backend");
    llama_backend_free();
}

// Load model
JNIEXPORT jboolean JNICALL
Java_com_saaya_ai_LlamaCpp_loadModel(
    JNIEnv* env, 
    jclass clazz,
    jstring modelPath,
    jint nCtx,
    jint nThreads
) {
    const char* model_path = env->GetStringUTFChars(modelPath, nullptr);
    
    LOGI("Loading model from: %s", model_path);
    LOGI("Context size: %d, Threads: %d", nCtx, nThreads);
    
    // Free previous model if exists
    if (g_llama_ctx) {
        if (g_llama_ctx->sampler) {
            llama_sampler_free(g_llama_ctx->sampler);
        }
        if (g_llama_ctx->ctx) {
            llama_free(g_llama_ctx->ctx);
        }
        if (g_llama_ctx->model) {
            llama_model_free(g_llama_ctx->model);
        }
    }
    
    g_llama_ctx = std::make_unique<LlamaContext>();
    g_llama_ctx->n_ctx = nCtx;
    g_llama_ctx->n_threads = nThreads;
    
    // Model parameters
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0; // CPU only
    model_params.use_mmap = true;  // Memory mapping for large models
    model_params.use_mlock = false;
    
    // Load model with new API
    g_llama_ctx->model = llama_model_load_from_file(model_path, model_params);
    
    env->ReleaseStringUTFChars(modelPath, model_path);
    
    if (!g_llama_ctx->model) {
        LOGE("Failed to load model");
        g_llama_ctx.reset();
        return JNI_FALSE;
    }
    
    // Context parameters
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = nCtx;
    ctx_params.n_threads = nThreads;
    ctx_params.n_threads_batch = nThreads;
    
    // Create context with new API
    g_llama_ctx->ctx = llama_init_from_model(g_llama_ctx->model, ctx_params);
    
    if (!g_llama_ctx->ctx) {
        LOGE("Failed to create context");
        llama_model_free(g_llama_ctx->model);
        g_llama_ctx.reset();
        return JNI_FALSE;
    }
    
    // Create sampler
    auto sparams = llama_sampler_chain_default_params();
    g_llama_ctx->sampler = llama_sampler_chain_init(sparams);
    
    llama_sampler_chain_add(g_llama_ctx->sampler, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(g_llama_ctx->sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(g_llama_ctx->sampler, llama_sampler_init_top_p(0.9f, 1));
    llama_sampler_chain_add(g_llama_ctx->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    
    LOGI("Model loaded successfully");
    return JNI_TRUE;
}

// Generate text (streaming)
JNIEXPORT jstring JNICALL
Java_com_saaya_ai_LlamaCpp_generateToken(
    JNIEnv* env,
    jclass clazz,
    jstring prompt,
    jint maxTokens
) {
    if (!g_llama_ctx || !g_llama_ctx->ctx) {
        LOGE("Context not initialized");
        return env->NewStringUTF("");
    }
    
    const char* prompt_cstr = env->GetStringUTFChars(prompt, nullptr);
    std::string prompt_str(prompt_cstr);
    env->ReleaseStringUTFChars(prompt, prompt_cstr);
    
    LOGI("Generating response for prompt: %s", prompt_str.c_str());
    
    // Tokenize prompt - get vocab from model
    const llama_vocab* vocab = llama_model_get_vocab(g_llama_ctx->model);
    std::vector<llama_token> tokens;
    tokens.resize(prompt_str.length() + 2);
    
    int n_tokens = llama_tokenize(
        vocab,
        prompt_str.c_str(),
        prompt_str.length(),
        tokens.data(),
        tokens.size(),
        true,  // add_bos
        false  // special
    );
    
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
            vocab,
            prompt_str.c_str(),
            prompt_str.length(),
            tokens.data(),
            tokens.size(),
            true,
            false
        );
    }
    
    tokens.resize(n_tokens);
    
    LOGI("Tokenized prompt: %d tokens", n_tokens);
    
    // Clear KV cache with new API
    llama_kv_cache_clear(g_llama_ctx->ctx);
    llama_sampler_reset(g_llama_ctx->sampler);
    
    // Process prompt
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    
    for (int i = 0; i < n_tokens; i++) {
        llama_batch_add(batch, tokens[i], i, {0}, false);
    }
    
    // Mark last token for logits
    if (batch.n_tokens > 0) {
        batch.logits[batch.n_tokens - 1] = true;
    }
    
    if (llama_decode(g_llama_ctx->ctx, batch) != 0) {
        LOGE("Failed to decode prompt");
        llama_batch_free(batch);
        return env->NewStringUTF("");
    }
    
    // Generate response
    std::string response;
    int n_cur = batch.n_tokens;
    int n_gen = 0;
    
    while (n_gen < maxTokens) {
        const llama_token new_token_id = llama_sampler_sample(g_llama_ctx->sampler, g_llama_ctx->ctx, -1);
        
        // Check for EOG with new API
        if (llama_vocab_is_eog(vocab, new_token_id)) {
            break;
        }
        
        // Get token text with new API
        char token_str[256];
        int n = llama_token_to_piece(vocab, new_token_id, token_str, sizeof(token_str), 0, false);
        
        if (n > 0) {
            response.append(token_str, n);
        }
        
        // Prepare next batch - manual clear
        batch.n_tokens = 0;
        llama_batch_add(batch, new_token_id, n_cur++, {0}, true);
        
        if (llama_decode(g_llama_ctx->ctx, batch) != 0) {
            LOGE("Failed to decode token");
            break;
        }
        
        n_gen++;
    }
    
    llama_batch_free(batch);
    
    LOGI("Generated %d tokens", n_gen);
    
    return env->NewStringUTF(response.c_str());
}

// Unload model
JNIEXPORT void JNICALL
Java_com_saaya_ai_LlamaCpp_unloadModel(JNIEnv* env, jclass clazz) {
    LOGI("Unloading model");
    
    if (g_llama_ctx) {
        if (g_llama_ctx->sampler) {
            llama_sampler_free(g_llama_ctx->sampler);
        }
        if (g_llama_ctx->ctx) {
            llama_free(g_llama_ctx->ctx);
        }
        if (g_llama_ctx->model) {
            llama_model_free(g_llama_ctx->model);
        }
        g_llama_ctx.reset();
    }
}

// Get model info
JNIEXPORT jstring JNICALL
Java_com_saaya_ai_LlamaCpp_getModelInfo(JNIEnv* env, jclass clazz) {
    if (!g_llama_ctx || !g_llama_ctx->model) {
        return env->NewStringUTF("No model loaded");
    }
    
    // Get vocab for vocab size
    const llama_vocab* vocab = llama_model_get_vocab(g_llama_ctx->model);
    int32_t n_vocab = llama_vocab_n_tokens(vocab);
    
    char buf[512];
    char desc[256];
    llama_model_desc(g_llama_ctx->model, desc, sizeof(desc));
    
    snprintf(buf, sizeof(buf), 
        "Model: %s\nContext: %d\nThreads: %d\nVocab: %d",
        desc,
        g_llama_ctx->n_ctx,
        g_llama_ctx->n_threads,
        n_vocab
    );
    
    return env->NewStringUTF(buf);
}

} // extern "C"
