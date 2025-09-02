#include "FontLoader.h"
#include "../AssetManager.h"
#include "../Managers/RuntimeFontManager.h"
#include "include/core/SkFontMgr.h"

#if defined(SK_BUILD_FOR_WIN)
#include "include/ports/SkTypeface_win.h"
#elif defined(SK_BUILD_FOR_MAC)
#include "include/ports/SkFontMgr_mac_ct.h"
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

#elif defined(SK_BUILD_FOR_UNIX)

    // ============================ KEY FIX ============================
    // Provide nullptr for both arguments to get the default behavior.
    return SkFontMgr_New_FontConfig(nullptr, nullptr);
    // ===============================================================

#else

    return SkFontMgr::RefEmpty();

#endif
}

static sk_sp<SkFontMgr> gFontMgr = CreatePlatformFontManager();

sk_sp<SkData> FontLoader::GetFontData(const AssetMetadata& fontMetadata)
{
    if (fontMetadata.type != AssetType::Font || !fontMetadata.importerSettings["encodedData"])
    {
        return nullptr;
    }

    const YAML::Binary binaryData = fontMetadata.importerSettings["encodedData"].as<YAML::Binary>();
    sk_sp<SkData> skData = SkData::MakeWithCopy(binaryData.data(), binaryData.size());

    return skData;
}

sk_sp<SkTypeface> FontLoader::LoadAsset(const AssetMetadata& metadata)
{
    sk_sp<SkData> fontData = GetFontData(metadata);
    if (!fontData)
    {
        return nullptr;
    }

    return gFontMgr->makeFromData(fontData);
}

sk_sp<SkTypeface> FontLoader::LoadAsset(const Guid& guid)
{
    const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(guid);
    if (!metadata || metadata->type != AssetType::Font)
    {
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

    return font;
}
