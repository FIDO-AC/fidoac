#include <jni.h>
#include <string>
#include <vector>
#include "depends/libsnark-fido-ac/example/main.hpp"

#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG,__VA_ARGS__)
#define LOGI(LOG_TAG,...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG,__VA_ARGS__)
#define LOGE(LOG_TAG,...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG,__VA_ARGS__)
#define LOGSIMPLE(...)

jbyteArray as_byte_array(JNIEnv *env, std::vector<unsigned char>vec) {
    unsigned char* buf = vec.data();

    jbyteArray array = env->NewByteArray (vec.size());
    env->SetByteArrayRegion (array, 0, vec.size(), reinterpret_cast<jbyte*>(buf));
    return array;
}

std::vector<unsigned char> as_unsigned_char_vector(JNIEnv *env, jbyteArray array) {
    int len = env->GetArrayLength (array);
    //__android_log_print(ANDROID_LOG_INFO, "FIDO_AC_ZK", "Length:%d", len);
    unsigned char* buf = new unsigned char[len];
    env->GetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));

    std::vector<unsigned char> buf_vector = std::vector<unsigned char>(buf, buf+len);
    //__android_log_print(ANDROID_LOG_INFO, "FIDO_AC_ZK", "Length:%d", buf_vector.size());

    return buf_vector; //return buf;
}

extern "C" JNIEXPORT jstring JNICALL
Java_anon_fidoac_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_anon_fidoac_MainActivity_snark_1sha256(JNIEnv *env,jobject thiz, jbyteArray dg1, jbyteArray client_nonce, jint ageLimit, jbyteArray proving_key,
                                            jbyteArray dg1_hash_for_testing, jbyteArray verf_key_for_testing) {
    std::vector<unsigned char> proof_data;
    compute_proof(proof_data, as_unsigned_char_vector(env,dg1), as_unsigned_char_vector(env,client_nonce),(int) ageLimit,
                         as_unsigned_char_vector(env,proving_key), as_unsigned_char_vector(env,dg1_hash_for_testing),
                  as_unsigned_char_vector(env,verf_key_for_testing));
    return as_byte_array(env, proof_data);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_anon_fidoac_MainActivity_00024Companion_FIDO_1AC_1veirfy(JNIEnv *env, jobject thiz,
                                                              jbyteArray zkproof,
                                                              jbyteArray randomized_hash,
                                                              jint age_limit,
                                                              jbyteArray verification_key) {
    return verify_proof(as_unsigned_char_vector(env,zkproof), as_unsigned_char_vector(env,randomized_hash),
                 (int) age_limit, as_unsigned_char_vector(env,verification_key));
}


extern "C"
JNIEXPORT jint JNICALL
Java_anon_fidoac_MainActivity_00024Companion_generate_1trusted_1setup(JNIEnv *env, jobject thiz,
                                                                      jbyteArray verification_key,
                                                                      jbyteArray proving_key,
                                                                      jint age_limit,
                                                                      jintArray arr_sizes) {
    std::vector<unsigned char> pk;
    std::vector<unsigned char> vk;
    std::vector<int> arr_size_vec;
    LOGI("FIDO_AC_ZK", "Generating_KEY");
    trusted_setup_generation(age_limit,vk,pk);
    LOGI("FIDO_AC_ZK", "Generating_KEY Complete");
    env->SetByteArrayRegion(verification_key, 0, vk.size(),
                            reinterpret_cast<const jbyte *>(vk.data()));
    env->SetByteArrayRegion(proving_key, 0, pk.size(), reinterpret_cast<const jbyte *>(pk.data()));
    arr_size_vec.push_back(vk.size());
    arr_size_vec.push_back(pk.size());
    env->SetIntArrayRegion(arr_sizes, 0, 2,
                           arr_size_vec.data());
    return 0;
}