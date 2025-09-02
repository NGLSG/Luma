#ifndef RUNTIMEANIMATIONCLIP_H
#define RUNTIMEANIMATIONCLIP_H
#include "AnimationClip.h"
#include "ComponentRegistry.h"
#include "IRuntimeAsset.h"

/**
 * @brief 表示一个运行时动画剪辑。
 * 这是一个运行时资产，封装了一个动画剪辑数据，并提供了访问和修改其属性的方法。
 */
class RuntimeAnimationClip : public IRuntimeAsset
{
    AnimationClip m_animationClip; ///< 实际的动画剪辑数据。

public:
    /**
     * @brief 获取内部动画剪辑的引用。
     * @return 对内部 AnimationClip 对象的引用。
     */
    AnimationClip& getAnimationClip() { return m_animationClip; }

    /**
     * @brief 获取内部动画剪辑的常量引用。
     * @return 对内部 AnimationClip 对象的常量引用。
     */
    const AnimationClip& getAnimationClip() const { return m_animationClip; }

    /**
     * @brief 获取动画剪辑的名称。
     * @return 动画剪辑的名称字符串。
     */
    std::string GetName() { return m_animationClip.Name; }

    /**
     * @brief 设置动画剪辑的名称。
     * @param newName 新的动画剪辑名称。
     */
    void SetName(const std::string& newName)
    {
        m_animationClip.Name = newName;
    }

    /**
     * @brief 构造一个新的运行时动画剪辑。
     * @param guid 运行时资产的全局唯一标识符。
     * @param animationClip 要封装的动画剪辑数据。
     */
    RuntimeAnimationClip(const Guid& guid, const AnimationClip& animationClip)
        : m_animationClip(animationClip)
    {
        m_sourceGuid = guid;
    }
};

#endif