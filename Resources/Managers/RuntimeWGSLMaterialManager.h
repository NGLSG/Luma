#ifndef LUMAENGINE_RUNTIMEWGSLMATERIALMANAGER_H
#define LUMAENGINE_RUNTIMEWGSLMATERIALMANAGER_H
#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeWGSLMaterial.h"


class RuntimeWGSLMaterialManager : public RuntimeAssetManagerBase<RuntimeWGSLMaterial>,
                                   public LazySingleton<RuntimeWGSLMaterialManager>
{
    friend class LazySingleton<RuntimeWGSLMaterialManager>;

private:
    RuntimeWGSLMaterialManager() = default ;
};


#endif //LUMAENGINE_RUNTIMEWGSLMATERIALMANAGER_H
