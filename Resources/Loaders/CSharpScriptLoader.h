#ifndef CSHARPSCRIPTLOADER_H
#define CSHARPSCRIPTLOADER_H


#include "IAssetLoader.h"
#include "../RuntimeAsset/RuntimeCSharpScript.h"

/**
 * @brief C# 脚本资源加载器。
 *
 * 负责从各种源加载 C# 脚本资产，并将其转换为运行时可用的 RuntimeCSharpScript 对象。
 */
class CSharpScriptLoader : public IAssetLoader<RuntimeCSharpScript>
{
public:
    /**
     * @brief 根据资产元数据加载 C# 脚本资产。
     * @param metadata 资产的元数据信息。
     * @return 指向加载的 C# 脚本运行时对象的智能指针。
     */
    sk_sp<RuntimeCSharpScript> LoadAsset(const AssetMetadata& metadata) override;

    /**
     * @brief 根据全局唯一标识符 (GUID) 加载 C# 脚本资产。
     * @param guid 资产的全局唯一标识符。
     * @return 指向加载的 C# 脚本运行时对象的智能指针。
     */
    sk_sp<RuntimeCSharpScript> LoadAsset(const Guid& guid) override;
};


#endif