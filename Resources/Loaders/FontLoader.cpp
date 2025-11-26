#include "FontLoader.h"
#include "../AssetManager.h"
#include "../Managers/RuntimeFontManager.h"
#include "include/core/SkFontMgr.h"
#include <mutex>

#if defined(SK_BUILD_FOR_WIN)
#include "include/ports/SkTypeface_win.h"
#elif defined(SK_BUILD_FOR_MAC)
#include "include/ports/SkFontMgr_mac_ct.h"
#elif defined(SK_BUILD_FOR_ANDROID)
#include "include/ports/SkFontMgr_android.h"
#include "include/ports/SkFontScanner_FreeType.h"
#include "include/core/SkStream.h"
#elif defined(SK_BUILD_FOR_UNIX)
#include "include/ports/SkFontMgr_fontconfig.h"
#include "include/ports/SkFontScanner_Fontations.h"
#endif


static sk_sp<SkFontMgr> CreatePlatformFontManager()
{
#if defined(SK_BUILD_FOR_WIN)
    
    return SkFontMgr_New_DirectWrite();

#elif defined(SK_BUILD_FOR_MAC)
    
    return SkFontMgr_New_CoreText(nullptr);

#elif defined(SK_BUILD_FOR_ANDROID)
    
    sk_sp<SkFontMgr> fontMgr = nullptr;

    try
    {
        fontMgr = SkFontMgr_New_Android(nullptr, SkFontScanner_Make_FreeType());

        if (fontMgr)
        {
            LogInfo("FontLoader: Android 字体管理器创建成功");
        }
    }
    catch (const std::exception& e)
    {
        LogError("FontLoader: 创建 Android 字体管理器时发生异常: {}", e.what());
        fontMgr = nullptr;
    }
    catch (...)
    {
        LogError("FontLoader: 创建 Android 字体管理器时发生未知异常");
        fontMgr = nullptr;
    }

    
    if (!fontMgr)
    {
        LogError("FontLoader: 获取默认字体管理器失败，使用空字体管理器");
        fontMgr = SkFontMgr::RefEmpty();
    }

    return fontMgr;

#elif defined(SK_BUILD_FOR_UNIX)
    
    return SkFontMgr_New_FontConfig(nullptr, nullptr);

#else
    
    return SkFontMgr::RefDefault();

#endif
}


static sk_sp<SkFontMgr> GetFontManager()
{
    
    static sk_sp<SkFontMgr> gFontMgr = nullptr;
    static std::once_flag initFlag;

    std::call_once(initFlag, []()
    {
        try
        {
            gFontMgr = CreatePlatformFontManager();
        }
        catch (const std::exception& e)
        {
            LogError("FontLoader: 初始化字体管理器时发生异常: {}", e.what());
            gFontMgr = nullptr;
        }
        catch (...)
        {
            LogError("FontLoader: 初始化字体管理器时发生未知异常");
            gFontMgr = nullptr;
        }

        if (!gFontMgr)
        {
            LogError("FontLoader: 平台字体管理器创建失败，使用空字体管理器");
            try
            {
                gFontMgr = SkFontMgr::RefEmpty();
            }
            catch (...)
            {
                LogError("FontLoader: 创建空字体管理器失败");
            }
        }
    });

    return gFontMgr;
}

sk_sp<SkData> FontLoader::GetFontData(const AssetMetadata& fontMetadata)
{
    if (fontMetadata.type != AssetType::Font)
    {
        LogError("FontLoader: 资产类型不是字体，类型: {}", static_cast<int>(fontMetadata.type));
        return nullptr;
    }

    if (!fontMetadata.importerSettings["encodedData"])
    {
        LogError("FontLoader: 字体元数据中缺少 encodedData 字段");
        return nullptr;
    }

    try
    {
        const YAML::Binary binaryData = fontMetadata.importerSettings["encodedData"].as<YAML::Binary>();

        if (binaryData.size() == 0)
        {
            LogError("FontLoader: 字体数据大小为 0");
            return nullptr;
        }

        sk_sp<SkData> skData = SkData::MakeWithCopy(binaryData.data(), binaryData.size());

        if (!skData)
        {
            LogError("FontLoader: 创建 SkData 失败");
            return nullptr;
        }

        return skData;
    }
    catch (const std::exception& e)
    {
        LogError("FontLoader: 获取字体数据时发生异常: {}", e.what());
        return nullptr;
    }
    catch (...)
    {
        LogError("FontLoader: 获取字体数据时发生未知异常");
        return nullptr;
    }
}

sk_sp<SkTypeface> FontLoader::LoadAsset(const AssetMetadata& metadata)
{

    
    sk_sp<SkData> fontData = GetFontData(metadata);
    if (!fontData)
    {
        LogError("FontLoader: 无法获取字体数据，GUID: {}", metadata.guid.ToString());
        return nullptr;
    }

    
    sk_sp<SkFontMgr> fontMgr = GetFontManager();
    if (!fontMgr)
    {
        LogError("FontLoader: 字体管理器未初始化，GUID: {}", metadata.guid.ToString());
        return nullptr;
    }

    
    sk_sp<SkTypeface> typeface = nullptr;
    try
    {
        typeface = fontMgr->makeFromData(fontData);
    }
    catch (const std::exception& e)
    {
        LogError("FontLoader: 创建 Typeface 时发生异常: {}，GUID: {}", e.what(), metadata.guid.ToString());
        return nullptr;
    }
    catch (...)
    {
        LogError("FontLoader: 创建 Typeface 时发生未知异常，GUID: {}", metadata.guid.ToString());
        return nullptr;
    }

    if (!typeface)
    {
        LogError("FontLoader: 无法从字体数据创建 Typeface，GUID: {}", metadata.guid.ToString());
        return nullptr;
    }
    return typeface;
}

sk_sp<SkTypeface> FontLoader::LoadAsset(const Guid& guid)
{

    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata)
    {
        LogError("FontLoader: 找不到字体元数据，GUID: {}", guid.ToString());
        return nullptr;
    }

    if (metadata->type != AssetType::Font)
    {
        LogError("FontLoader: 资源类型不匹配，期望: Font，实际: {}，GUID: {}",
                 static_cast<int>(metadata->type), guid.ToString());
        return nullptr;
    }

    sk_sp<SkTypeface> typeface;

    
    if (RuntimeFontManager::GetInstance().TryGetAsset(guid, typeface))
    {
        return typeface;
    }

    
    auto font = LoadAsset(*metadata);
    if (font)
    {
        
        RuntimeFontManager::GetInstance().TryAddOrUpdateAsset(guid, font);
    }
    else
    {
        LogError("FontLoader: 字体加载失败，GUID: {}", guid.ToString());
    }

    return font;
}
