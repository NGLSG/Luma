#ifndef RUNTIMEPHYSISCSMATERIALMANAGER_H
#define RUNTIMEPHYSISCSMATERIALMANAGER_H


#include "IRuntimeAssetManager.h"
#include "../RuntimeAsset/RuntimePhysicsMaterial.h"

/**
 * @brief 运行时物理材质管理器。
 *
 * 负责管理运行时物理材质资源，并确保其作为单例存在。
 */
class RuntimePhysicsMaterialManager
    : public RuntimeAssetManagerBase<RuntimePhysicsMaterial>,
      public LazySingleton<RuntimePhysicsMaterialManager>
{
public:
    /**
     * @brief 声明 LazySingleton 模板类为友元，允许其访问私有构造函数。
     */
    friend class LazySingleton<RuntimePhysicsMaterialManager>;

private:
    /**
     * @brief 默认构造函数，私有化以强制单例模式。
     */
    RuntimePhysicsMaterialManager() = default;
};


#endif