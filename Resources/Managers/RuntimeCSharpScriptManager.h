#ifndef RUNTIMECSHARPSCRIPTMANAGER_H
#define RUNTIMECSHARPSCRIPTMANAGER_H

#include "IRuntimeAssetManager.h"
#include "../RuntimeAsset/RuntimeCSharpScript.h"

/**
 * @brief 运行时C#脚本管理器。
 * @details 负责管理和加载运行时C#脚本资源。
 */
class RuntimeCSharpScriptManager
    : public RuntimeAssetManagerBase<RuntimeCSharpScript>,
      public LazySingleton<RuntimeCSharpScriptManager>
{
public:
    /**
     * @brief 声明LazySingleton类为友元，允许其访问本类的私有成员。
     */
    friend class LazySingleton<RuntimeCSharpScriptManager>;

private:
    /**
     * @brief 默认构造函数，私有化以确保单例模式。
     */
    RuntimeCSharpScriptManager() = default;
};

#endif