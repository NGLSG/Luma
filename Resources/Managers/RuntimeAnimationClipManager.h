/**
 * @brief 运行时动画剪辑管理器。
 *
 * 该类负责管理运行时动画剪辑资源。它继承自 `RuntimeAssetManagerBase`，
 * 并实现了 `LazySingleton` 模式，确保在整个应用程序中只有一个实例，并按需创建。
 */
#ifndef RUNTIMEANIMATIONCLIPMANAGER_H
#define RUNTIMEANIMATIONCLIPMANAGER_H
#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeAnimationClip.h"


class RuntimeAnimationClipManager
    : public RuntimeAssetManagerBase<RuntimeAnimationClip>,
      public LazySingleton<RuntimeAnimationClipManager>
{
public:
    friend class LazySingleton<RuntimeAnimationClipManager>;

private:
    RuntimeAnimationClipManager() = default;
};


#endif