#include "AndroidPermissions.h"

#include <memory>

#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>
#include <jni.h>
#include <condition_variable>
#include <mutex>
#endif

namespace Platform::Android
{
#if defined(__ANDROID__)
namespace
{
    constexpr const char* kPermissionHelperClass = "com/lumaengine/lumaandroid/LumaPermissionUtils";

    jobjectArray CreateJavaStringArray(JNIEnv* env, const std::vector<std::string>& permissions)
    {
        jclass stringClass = env->FindClass("java/lang/String");
        if (!stringClass)
        {
            env->ExceptionClear();
            return nullptr;
        }

        jobjectArray array = env->NewObjectArray(static_cast<jsize>(permissions.size()), stringClass, nullptr);
        env->DeleteLocalRef(stringClass);
        if (!array)
        {
            env->ExceptionClear();
            return nullptr;
        }

        for (jsize i = 0; i < static_cast<jsize>(permissions.size()); ++i)
        {
            jstring entry = env->NewStringUTF(permissions[static_cast<size_t>(i)].c_str());
            env->SetObjectArrayElement(array, i, entry);
            env->DeleteLocalRef(entry);
        }
        return array;
    }

    struct PermissionRequestState
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool completed = false;
        bool granted = false;
    };

    void SignalState(PermissionRequestState* state, bool success)
    {
        if (!state)
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->completed = true;
            state->granted = success;
        }
        state->cv.notify_all();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_lumaengine_lumaandroid_LumaPermissionUtils_nativeOnPermissionResult(JNIEnv*, jclass, jlong statePtr, jboolean granted)
{
    auto* state = reinterpret_cast<PermissionRequestState*>(statePtr);
    SignalState(state, granted == JNI_TRUE);
}
#endif

bool HasPermissions(const std::vector<std::string>& permissions)
{
#if !defined(__ANDROID__)
    (void)permissions;
    return true;
#else
    if (permissions.empty())
    {
        return true;
    }

    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env)
    {
        return false;
    }

    jclass helperClass = env->FindClass(kPermissionHelperClass);
    if (!helperClass)
    {
        env->ExceptionClear();
        return false;
    }

    jmethodID hasPermissionsMethod = env->GetStaticMethodID(helperClass, "hasPermissions", "([Ljava/lang/String;)Z");
    if (!hasPermissionsMethod)
    {
        env->ExceptionClear();
        env->DeleteLocalRef(helperClass);
        return false;
    }

    jobjectArray array = CreateJavaStringArray(env, permissions);
    if (!array)
    {
        env->DeleteLocalRef(helperClass);
        return false;
    }

    jboolean result = env->CallStaticBooleanMethod(helperClass, hasPermissionsMethod, array);
    bool success = (result == JNI_TRUE);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        success = false;
    }
    env->DeleteLocalRef(array);
    env->DeleteLocalRef(helperClass);
    return success;
#endif
}

bool AcquirePermissions(const std::vector<std::string>& permissions)
{
#if !defined(__ANDROID__)
    (void)permissions;
    return true;
#else
    if (permissions.empty())
    {
        return true;
    }

    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env)
    {
        return false;
    }

    jclass helperClass = env->FindClass(kPermissionHelperClass);
    if (!helperClass)
    {
        env->ExceptionClear();
        return false;
    }

    jmethodID acquireMethod = env->GetStaticMethodID(helperClass, "acquirePermissions", "(J[Ljava/lang/String;)Z");
    if (!acquireMethod)
    {
        env->ExceptionClear();
        env->DeleteLocalRef(helperClass);
        return false;
    }

    jobjectArray array = CreateJavaStringArray(env, permissions);
    if (!array)
    {
        env->DeleteLocalRef(helperClass);
        return false;
    }

    auto state = std::make_unique<PermissionRequestState>();

    env->CallStaticBooleanMethod(helperClass, acquireMethod, reinterpret_cast<jlong>(state.get()), array);
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        env->DeleteLocalRef(array);
        env->DeleteLocalRef(helperClass);
        return false;
    }
    env->DeleteLocalRef(array);
    env->DeleteLocalRef(helperClass);

    std::unique_lock<std::mutex> lock(state->mutex);
    state->cv.wait(lock, [&]() { return state->completed; });
    return state->granted;
#endif
}
}
