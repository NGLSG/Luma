#ifndef LUMAENGINE_RUNTIMESHADERMANAGER_H
#define LUMAENGINE_RUNTIMESHADERMANAGER_H
#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeShader.h"


class RuntimeShaderManager : public RuntimeAssetManagerBase<RuntimeShader>, public LazySingleton<RuntimeShaderManager>
{
    friend class LazySingleton<RuntimeShaderManager>;

private:
    RuntimeShaderManager() = default;
};


#endif //LUMAENGINE_RUNTIMESHADERMANAGER_H
