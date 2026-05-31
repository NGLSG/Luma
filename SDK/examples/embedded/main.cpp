#include <cstdio>

// LumaEngine C API
#include <LumaEngine/EngineEntry.h>
#include <LumaEngine/Luma_CAPI.h>

int main(int argc, char** argv) {
    printf("LumaEngine Embedded Example\n");
    printf("===========================\n\n");

    // Method 1: Full runtime (simplest)
    // This launches the complete game runtime with its own window.
    // Suitable for standalone games.
    printf("Launching LumaEngine Game Runtime...\n");
    LumaEngine_Game_Entry(argc, argv, "./", "");

    return 0;
}
