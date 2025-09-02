/**
 * @brief 表示一个运行时规则瓦片资产。
 *
 * 该类继承自 IRuntimeAsset，用于在运行时表示和管理 RuleTile 的数据。
 */
#ifndef LUMAENGINE_RUNTIMERULETILE_H
#define LUMAENGINE_RUNTIMERULETILE_H
#include "IRuntimeAsset.h"
#include "RuleTile.h"


class RuntimeRuleTile : public IRuntimeAsset
{
public:
    /**
     * @brief 构造函数，用于创建一个运行时规则瓦片实例。
     * @param sourceGuid 源资产的全局唯一标识符。
     * @param data 规则瓦片资产数据。
     */
    RuntimeRuleTile(const Guid& sourceGuid, RuleTileAssetData data)
        : m_data(std::move(data))
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 获取规则瓦片资产数据。
     * @return 常量引用，指向规则瓦片资产数据。
     */
    const RuleTileAssetData& GetData() const { return m_data; }

private:
    RuleTileAssetData m_data; ///< 存储规则瓦片资产数据。
};

#endif