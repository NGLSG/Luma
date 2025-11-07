



#include "EngineEntry.h"

#include <jni.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>


extern "C" int SDL_main(int argc, char *argv[]) {
    
    const char *basedir = std::getenv("LUMA_BASEDIR");
    if (basedir && basedir[0] != '\0') {
        if (chdir(basedir) != 0) {
            std::perror("chdir(LUMA_BASEDIR) failed");
        }
    }

#ifdef LUMA_EDITOR
    return LumaEngine_Editor_Entry(argc, argv,basedir);
#else
    return LumaEngine_Game_Entry(argc, argv, basedir);
#endif
}
