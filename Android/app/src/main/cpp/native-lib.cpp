#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring
Java_com_lumaengine_android_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string cpp_version;
    if (__cplusplus == 202100L) cpp_version = "C++23";
    else if (__cplusplus == 202002L) cpp_version = "C++20";
    else if (__cplusplus == 201703L) cpp_version = "C++17";
    else if (__cplusplus == 201402L) cpp_version = "C++14";
    else if (__cplusplus == 201103L) cpp_version = "C++11";
    else if (__cplusplus == 199711L) cpp_version = "C++98";
    else cpp_version = "Unknown C++ version";

    std::string hello = "C++ version is: " + cpp_version;
    return env->NewStringUTF(hello.c_str());
}