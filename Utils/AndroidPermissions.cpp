#include "AndroidPermissions.h"
#include <memory>

#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>
#include <jni.h>
#include <condition_variable>
#include <mutex>
#endif

#include <sstream>

namespace Platform::Android
{
#if defined(__ANDROID__)
namespace
{
    constexpr const char* kPermissionHelperClass = "com/lumaengine/lumaandroid/LumaPermissionUtils";

    struct JavaRefs
    {
        jclass helperClass = nullptr;
        jmethodID hasPermissionsMethod = nullptr;
        jmethodID acquirePermissionsMethod = nullptr;
        std::mutex mutex;
        bool initialized = false;
    };

    static JavaRefs g_javaRefs;

    static bool EnsureJavaRefsInitialized(JNIEnv* env)
    {
        std::lock_guard<std::mutex> lock(g_javaRefs.mutex);
        if (g_javaRefs.initialized)
            return true;

        jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
        if (!activity)
        {
            LogError("Failed to get Android activity");
            return false;
        }

        jclass activityClass = env->GetObjectClass(activity);
        jmethodID getClassLoaderMethod = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jobject classLoader = env->CallObjectMethod(activity, getClassLoaderMethod);
        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
        jmethodID loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

        jstring classNameStr = env->NewStringUTF(kPermissionHelperClass);
        jclass localClass = static_cast<jclass>(env->CallObjectMethod(classLoader, loadClassMethod, classNameStr));
        env->DeleteLocalRef(classNameStr);
        env->DeleteLocalRef(classLoaderClass);
        env->DeleteLocalRef(activityClass);

        if (!localClass)
        {
            LogError("Failed to load class: {}", kPermissionHelperClass);
            if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
            return false;
        }

        g_javaRefs.helperClass = static_cast<jclass>(env->NewGlobalRef(localClass));
        env->DeleteLocalRef(localClass);

        if (!g_javaRefs.helperClass)
        {
            LogError("Failed to create global ref for helper class");
            return false;
        }

        g_javaRefs.hasPermissionsMethod = env->GetStaticMethodID(
            g_javaRefs.helperClass, "hasPermissions", "([Ljava/lang/String;)Z");
        g_javaRefs.acquirePermissionsMethod = env->GetStaticMethodID(
            g_javaRefs.helperClass, "acquirePermissions", "(J[Ljava/lang/String;)Z");

        if (!g_javaRefs.hasPermissionsMethod || !g_javaRefs.acquirePermissionsMethod)
        {
            LogError("Failed to get Java method IDs");
            if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
            env->DeleteGlobalRef(g_javaRefs.helperClass);
            g_javaRefs.helperClass = nullptr;
            return false;
        }

        g_javaRefs.initialized = true;
        LogInfo("JavaRefs initialized successfully");
        return true;
    }

    jobjectArray CreateJavaStringArray(JNIEnv* env, const std::vector<std::string>& permissions)
    {
        jclass stringClass = env->FindClass("java/lang/String");
        if (!stringClass)
        {
            LogError("Failed to find java/lang/String class");
            if (env->ExceptionCheck()) env->ExceptionClear();
            return nullptr;
        }

        jobjectArray array = env->NewObjectArray(static_cast<jsize>(permissions.size()), stringClass, nullptr);
        env->DeleteLocalRef(stringClass);
        if (!array)
        {
            LogError("Failed to create Java string array");
            if (env->ExceptionCheck()) env->ExceptionClear();
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
        if (!state) return;
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
    if (permissions.empty()) return true;

    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env)
    {
        LogError("JNIEnv unavailable");
        return false;
    }

    if (!EnsureJavaRefsInitialized(env))
        return false;

    jobjectArray array = CreateJavaStringArray(env, permissions);
    if (!array)
        return false;

    jboolean result = env->CallStaticBooleanMethod(g_javaRefs.helperClass, g_javaRefs.hasPermissionsMethod, array);
    bool success = (result == JNI_TRUE);
    if (env->ExceptionCheck())
    {
        LogError("Java exception during hasPermissions");
        env->ExceptionDescribe();
        env->ExceptionClear();
        success = false;
    }
    env->DeleteLocalRef(array);
    return success;
#endif
}

bool AcquirePermissions(const std::vector<std::string>& permissions)
{
#if !defined(__ANDROID__)
    (void)permissions;
    return true;
#else
    if (permissions.empty()) return true;

    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env)
    {
        LogError("JNIEnv unavailable");
        return false;
    }

    if (!EnsureJavaRefsInitialized(env))
        return false;

    jobjectArray array = CreateJavaStringArray(env, permissions);
    if (!array)
        return false;

    auto state = std::make_unique<PermissionRequestState>();

    env->CallStaticBooleanMethod(g_javaRefs.helperClass, g_javaRefs.acquirePermissionsMethod,
                                  reinterpret_cast<jlong>(state.get()), array);
    if (env->ExceptionCheck())
    {
        LogError("Java exception during acquirePermissions");
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(array);
        return false;
    }
    env->DeleteLocalRef(array);

    std::unique_lock<std::mutex> lock(state->mutex);
    state->cv.wait(lock, [&]() { return state->completed; });
    return state->granted;
#endif
}
}
