#include "EngineEntry.h"

int main(int argc, char* argv[])
{
#ifdef LUMA_EDITOR
    return LumaEngine_Editor_Entry(argc, argv, nullptr, nullptr);
#else
#if defined(OPENSSL_VERSION_NUMBER) && (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    OPENSSL_init_crypto(OPENSSL_INIT_NO_ATEXIT, nullptr);
#endif

    std::setlocale(LC_ALL, ".UTF8");
#if (defined(_WIN32) || defined(_WIN64))

    if (!SetDllDirectoryW(L"GameData"))
    {
        std::cerr << "Fatal Error: Could not set DLL search directory to 'GameData'.\n";
        return -1;
    }
#endif

    return LumaEngine_Game_Entry(argc, argv, nullptr, nullptr);
#endif
}
