/**
 * @brief 表示运行时瓦片资产。
 *
 * RuntimeTile 类继承自 IRuntimeAsset，用于在运行时表示一个瓦片资产。
 * 它封装了瓦片资产的原始数据。
 */
#ifndef LUMAENGINE_RUNTIMETILE_H
#define LUMAENGINE_RUNTIMETILE_H
#pragma once
#include "IRuntimeAsset.h"
#include "Tile.h"


class RuntimeTile : public IRuntimeAsset
{
public:
    /**
     * @brief 构造函数，用于创建运行时瓦片对象。
     * @param sourceGuid 源资产的全局唯一标识符。
     * @param data 瓦片资产数据。
     */
    RuntimeTile(const Guid& sourceGuid, TileAssetData data)
        : m_data(std::move(data))
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 获取瓦片资产的原始数据。
     * @return 对瓦片资产数据的常量引用。
     */
    const TileAssetData& GetData() const { return m_data; }

private:
    TileAssetData m_data; ///< 存储瓦片资产数据。
};

#endif