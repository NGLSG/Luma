#include <jni.h>
#include <cstdlib>

#if defined(__ANDROID__)
#include <unistd.h>
#endif

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_SDLActivity_nativeSetenv(JNIEnv* env, jclass, jstring name, jstring value)
{
    if (!env || !name)
    {
        return;
    }

    const char* cname = env->GetStringUTFChars(name, nullptr);
    const char* cvalue = value ? env->GetStringUTFChars(value, nullptr) : nullptr;

    if (cname)
    {
#if defined(__ANDROID__) || defined(__linux__) || defined(_WIN32)
        if (cvalue)
        {
#if defined(_WIN32)
            _putenv_s(cname, cvalue);
#else
            setenv(cname, cvalue, 1);
#endif
        }
        else
        {
#if defined(_WIN32)
            _putenv_s(cname, "");
#else
            unsetenv(cname);
#endif
        }
#endif
    }

    if (cname)
    {
        env->ReleaseStringUTFChars(name, cname);
    }
    if (cvalue)
    {
        env->ReleaseStringUTFChars(value, cvalue);
    }
}
