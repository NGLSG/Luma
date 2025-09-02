/**
 * @brief 表示运行时瓦片集资产。
 *
 * RuntimeTileset 类继承自 IRuntimeAsset，用于在运行时管理和访问瓦片集数据。
 */
#ifndef LUMAENGINE_RUNTIMETILESET_H
#define LUMAENGINE_RUNTIMETILESET_H
#include "IRuntimeAsset.h"
#include "Tileset.h"

class RuntimeTileset : public IRuntimeAsset
{
public:
    /**
     * @brief 构造函数，用于创建运行时瓦片集实例。
     * @param sourceGuid 源资产的全局唯一标识符。
     * @param data 瓦片集数据。
     */
    RuntimeTileset(const Guid& sourceGuid, TilesetData data)
        : m_data(std::move(data))
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 获取瓦片集数据。
     * @return 对瓦片集数据的常量引用。
     */
    const TilesetData& GetData() const { return m_data; }

private:
    TilesetData m_data; ///< 存储瓦片集数据。
};
#endif