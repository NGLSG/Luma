#ifndef RUNTIMEANIMATIONCONTROLLERMANAGER_H
#define RUNTIMEANIMATIONCONTROLLERMANAGER_H

#include "IRuntimeAssetManager.h"
#include "RuntimeAsset/RuntimeAnimationController.h"
#include "LazySingleton.h"

/**
 * @brief 运行时动画控制器管理器。
 *
 * 该类负责管理运行时动画控制器资产，并采用懒加载单例模式。
 */
class RuntimeAnimationControllerManager
    : public RuntimeAssetManagerBase<RuntimeAnimationController>,
      public LazySingleton<RuntimeAnimationControllerManager>
{
public:
    /// 声明LazySingleton为友元类，允许其访问私有成员。
    friend class LazySingleton<RuntimeAnimationControllerManager>;

private:
    /**
     * @brief 私有默认构造函数。
     *
     * 确保只能通过LazySingleton模式获取实例。
     */
    RuntimeAnimationControllerManager() = default;
};
#endif