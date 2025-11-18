#ifndef MATERIALLOADER_H
#define MATERIALLOADER_H

#include "IAssetLoader.h"
#include "../../Renderer/RenderComponent.h"
#include "../RuntimeAsset/RuntimeWGSLMaterial.h"
#include <variant>

// 材质变体类型：支持SkSL（已弃用）和WGSL材质
using MaterialVariant = std::variant<sk_sp<Material>, sk_sp<RuntimeWGSLMaterial>>;

/**
 * @brief 材质加载器，用于加载材质资源。
 *
 * 继承自 IAssetLoader 接口，专门处理 Material 类型的资产加载。
 * 支持SkSL（向后兼容）和WGSL材质。
 */
class MaterialLoader : public IAssetLoader<Material>
{
public:
    /**
     * @brief 根据资产元数据加载材质资源（SkSL - 已弃用）。
     * @param metadata 资产的元数据信息。
     * @return 指向加载的材质资源的共享指针。
     */
    sk_sp<Material> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符（GUID）加载材质资源（SkSL - 已弃用）。
     * @param Guid 资产的全局唯一标识符。
     * @return 指向加载的材质资源的共享指针。
     */
    sk_sp<Material> LoadAsset(const Guid& Guid) override;

    /**
     * @brief 加载WGSL材质（推荐）
     * @param metadata 资产的元数据信息
     * @param context NutContext图形上下文
     * @return WGSL材质的sk_sp智能指针
     */
    sk_sp<RuntimeWGSLMaterial> LoadWGSLMaterial(
        const AssetMetadata& metadata,
        const std::shared_ptr<Nut::NutContext>& context
    );

    /**
     * @brief 根据GUID加载WGSL材质
     * @param guid 资产的全局唯一标识符
     * @param context NutContext图形上下文
     * @return WGSL材质的sk_sp智能指针
     */
    sk_sp<RuntimeWGSLMaterial> LoadWGSLMaterial(
        const Guid& guid,
        const std::shared_ptr<Nut::NutContext>& context
    );
};


#endif