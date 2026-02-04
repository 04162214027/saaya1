#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <thread>
#include "llama.h"
#include "common.h"
#include "sampling.h"

#define TAG "LlamaNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

struct llama_context_wrapper {
    llama_model* model;
    llama_context* ctx;
    llama_sampler* sampler;
};

extern "C" JNIEXPORT jlong JNICALL
Java_com_saaya_ai_LlamaNative_loadModel(
        JNIEnv* env,
        jobject /* this */,
        jstring modelPath,
        jint nCtx,
        jint nThreads) {
    
    const char* model_path = env->GetStringUTFChars(modelPath, nullptr);
    LOGD("Loading model from: %s", model_path);
    
    // Initialize backend
    llama_backend_init();
    
    // Load model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0; // CPU only for compatibility
    
    llama_model* model = llama_load_model_from_file(model_path, model_params);
    env->ReleaseStringUTFChars(modelPath, model_path);
    
    if (!model) {
        LOGE("Failed to load model");
        return 0;
    }
    
    // Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = nCtx;
    ctx_params.n_threads = nThreads;
    ctx_params.n_threads_batch = nThreads;
    
    llama_context* ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        LOGE("Failed to create context");
        llama_free_model(model);
        return 0;
    }
    
    // Create sampler
    auto sparams = llama_sampler_chain_default_params();
    llama_sampler* sampler = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    
    // Wrap in struct
    auto* wrapper = new llama_context_wrapper{model, ctx, sampler};
    
    LOGD("Model loaded successfully");
    return reinterpret_cast<jlong>(wrapper);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_saaya_ai_LlamaNative_completion(
        JNIEnv* env,
        jobject /* this */,
        jlong modelPtr,
        jstring prompt,
        jint maxTokens,
        jfloat temperature,
        jint topK,
        jfloat topP) {
    
    auto* wrapper = reinterpret_cast<llama_context_wrapper*>(modelPtr);
    if (!wrapper || !wrapper->model || !wrapper->ctx) {
        LOGE("Invalid model pointer");
        return env->NewStringUTF("");
    }
    
    const char* prompt_text = env->GetStringUTFChars(prompt, nullptr);
    LOGD("Generating completion for: %s", prompt_text);
    
    // Tokenize prompt
    std::vector<llama_token> tokens;
    const int n_prompt_tokens = -llama_tokenize(
        wrapper->model,
        prompt_text,
        strlen(prompt_text),
        nullptr,
        0,
        true,
        true
    );
    tokens.resize(n_prompt_tokens);
    llama_tokenize(
        wrapper->model,
        prompt_text,
        strlen(prompt_text),
        tokens.data(),
        tokens.size(),
        true,
        true
    );
    
    env->ReleaseStringUTFChars(prompt, prompt_text);
    
    // Update sampler parameters
    llama_sampler_free(wrapper->sampler);
    auto sparams = llama_sampler_chain_default_params();
    wrapper->sampler = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_temp(temperature));
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_top_k(topK));
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_top_p(topP, 1));
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    
    // Evaluate prompt
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    if (llama_decode(wrapper->ctx, batch) != 0) {
        LOGE("Failed to decode prompt");
        return env->NewStringUTF("");
    }
    
    // Generate tokens
    std::string result;
    int n_generated = 0;
    
    while (n_generated < maxTokens) {
        // Sample next token
        llama_token new_token_id = llama_sampler_sample(wrapper->sampler, wrapper->ctx, -1);
        
        // Check for EOS
        if (llama_token_is_eog(wrapper->model, new_token_id)) {
            break;
        }
        
        // Decode token to text
        char buf[256];
        int n = llama_token_to_piece(wrapper->model, new_token_id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            LOGE("Failed to decode token");
            break;
        }
        result.append(buf, n);
        
        // Prepare next batch
        batch = llama_batch_get_one(&new_token_id, 1);
        if (llama_decode(wrapper->ctx, batch) != 0) {
            LOGE("Failed to decode token");
            break;
        }
        
        n_generated++;
    }
    
    LOGD("Generated %d tokens", n_generated);
    return env->NewStringUTF(result.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_saaya_ai_LlamaNative_completionStream(
        JNIEnv* env,
        jobject /* this */,
        jlong modelPtr,
        jstring prompt,
        jint maxTokens,
        jfloat temperature,
        jint topK,
        jfloat topP,
        jobject callback) {
    
    auto* wrapper = reinterpret_cast<llama_context_wrapper*>(modelPtr);
    if (!wrapper || !wrapper->model || !wrapper->ctx) {
        LOGE("Invalid model pointer");
        return;
    }
    
    // Get callback method
    jclass callbackClass = env->GetObjectClass(callback);
    jmethodID onTokenMethod = env->GetMethodID(callbackClass, "onToken", "(Ljava/lang/String;)V");
    if (!onTokenMethod) {
        LOGE("Failed to find onToken method");
        return;
    }
    
    const char* prompt_text = env->GetStringUTFChars(prompt, nullptr);
    LOGD("Streaming completion for: %s", prompt_text);
    
    // Tokenize prompt
    std::vector<llama_token> tokens;
    const int n_prompt_tokens = -llama_tokenize(
        wrapper->model,
        prompt_text,
        strlen(prompt_text),
        nullptr,
        0,
        true,
        true
    );
    tokens.resize(n_prompt_tokens);
    llama_tokenize(
        wrapper->model,
        prompt_text,
        strlen(prompt_text),
        tokens.data(),
        tokens.size(),
        true,
        true
    );
    
    env->ReleaseStringUTFChars(prompt, prompt_text);
    
    // Update sampler
    llama_sampler_free(wrapper->sampler);
    auto sparams = llama_sampler_chain_default_params();
    wrapper->sampler = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_temp(temperature));
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_top_k(topK));
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_top_p(topP, 1));
    llama_sampler_chain_add(wrapper->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    
    // Evaluate prompt
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    if (llama_decode(wrapper->ctx, batch) != 0) {
        LOGE("Failed to decode prompt");
        return;
    }
    
    // Stream tokens
    int n_generated = 0;
    while (n_generated < maxTokens) {
        // Sample next token
        llama_token new_token_id = llama_sampler_sample(wrapper->sampler, wrapper->ctx, -1);
        
        // Check for EOS
        if (llama_token_is_eog(wrapper->model, new_token_id)) {
            break;
        }
        
        // Decode token
        char buf[256];
        int n = llama_token_to_piece(wrapper->model, new_token_id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            LOGE("Failed to decode token");
            break;
        }
        
        // Call Java callback
        jstring token = env->NewStringUTF(std::string(buf, n).c_str());
        env->CallVoidMethod(callback, onTokenMethod, token);
        env->DeleteLocalRef(token);
        
        // Prepare next batch
        batch = llama_batch_get_one(&new_token_id, 1);
        if (llama_decode(wrapper->ctx, batch) != 0) {
            LOGE("Failed to decode token");
            break;
        }
        
        n_generated++;
    }
    
    LOGD("Streamed %d tokens", n_generated);
}

extern "C" JNIEXPORT void JNICALL
Java_com_saaya_ai_LlamaNative_freeModel(
        JNIEnv* env,
        jobject /* this */,
        jlong modelPtr) {
    
    auto* wrapper = reinterpret_cast<llama_context_wrapper*>(modelPtr);
    if (wrapper) {
        LOGD("Freeing model resources");
        if (wrapper->sampler) {
            llama_sampler_free(wrapper->sampler);
        }
        if (wrapper->ctx) {
            llama_free(wrapper->ctx);
        }
        if (wrapper->model) {
            llama_free_model(wrapper->model);
        }
        delete wrapper;
        llama_backend_free();
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_saaya_ai_LlamaNative_getContextSize(
        JNIEnv* env,
        jobject /* this */,
        jlong modelPtr) {
    
    auto* wrapper = reinterpret_cast<llama_context_wrapper*>(modelPtr);
    if (wrapper && wrapper->ctx) {
        return llama_n_ctx(wrapper->ctx);
    }
    return 0;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_saaya_ai_LlamaNative_isModelLoaded(
        JNIEnv* env,
        jobject /* this */,
        jlong modelPtr) {
    
    auto* wrapper = reinterpret_cast<llama_context_wrapper*>(modelPtr);
    return (wrapper && wrapper->model && wrapper->ctx) ? JNI_TRUE : JNI_FALSE;
}
