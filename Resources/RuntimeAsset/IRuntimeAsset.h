/**
 * @file IRuntimeAsset.h
 * @brief 定义了运行时资产的接口。
 */
#ifndef IRUNTIMEASSET_H
#define IRUNTIMEASSET_H
#pragma once

#include "../../Utils/Guid.h"
#include <include/core/SkRefCnt.h>

/**
 * @brief 运行时资产接口。
 *
 * 此接口定义了所有运行时资产的基本行为和属性，例如其源全局唯一标识符 (GUID)。
 * 它继承自 SkRefCnt，表示它是一个引用计数的对象。
 */
class IRuntimeAsset : public SkRefCnt
{
public:
    /**
     * @brief 虚析构函数。
     *
     * 确保派生类能够正确清理资源。
     */
    virtual ~IRuntimeAsset() = default;

    /**
     * @brief 获取此运行时资产的源全局唯一标识符 (GUID)。
     *
     * 源 GUID 用于标识资产的原始来源或定义。
     *
     * @return 返回源 GUID 的常量引用。
     */
    [[nodiscard]] const Guid& GetSourceGuid() const { return m_sourceGuid; }

protected:
    Guid m_sourceGuid; ///< 此运行时资产的源全局唯一标识符 (GUID)。
};
#endif