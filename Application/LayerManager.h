#ifndef LAYERMANAGER_H
#define LAYERMANAGER_H

#include <string>
#include <array>
#include "ProjectSettings.h"

/**
 * @brief 层管理器，管理32个层的名称
 * 
 * 类似 Unity 的 Layer 系统，提供层名称的管理
 * 层 0-31 对应 LayerMask 的 32 个位
 * 层名称存储在 ProjectSettings 中
 */
class LayerManager
{
public:
    static constexpr int MAX_LAYERS = 32;
    
    /// 获取指定层的名称（从 ProjectSettings 读取）
    static std::string GetLayerName(int layer)
    {
        if (layer < 0 || layer >= MAX_LAYERS) return "";
        auto& ps = ProjectSettings::GetInstance();
        return ps.GetLayerName(layer);
    }
    
    /// 设置指定层的名称（保存到 ProjectSettings）
    static void SetLayerName(int layer, const std::string& name)
    {
        if (layer < 0 || layer >= MAX_LAYERS) return;
        auto& ps = ProjectSettings::GetInstance();
        ps.SetLayerName(layer, name);
        ps.Save();
    }
    
    /// 根据名称获取层索引，未找到返回 -1
    static int NameToLayer(const std::string& name)
    {
        if (name.empty()) return -1;
        auto& ps = ProjectSettings::GetInstance();
        const auto& layers = ps.GetLayers();
        for (const auto& [idx, layerName] : layers)
        {
            if (layerName == name)
                return idx;
        }
        return -1;
    }
    
    /// 获取层掩码（单层）
    static uint32_t GetMask(int layer)
    {
        if (layer < 0 || layer >= MAX_LAYERS) return 0;
        return 1u << layer;
    }
    
    /// 获取层掩码（通过名称）
    static uint32_t GetMask(const std::string& layerName)
    {
        int layer = NameToLayer(layerName);
        if (layer < 0) return 0;
        return 1u << layer;
    }
    
    /// 确保默认层名称已初始化
    static void EnsureDefaults()
    {
        auto& ps = ProjectSettings::GetInstance();
        ps.EnsureDefaultLayers();
    }
    
    /// 检查层是否有自定义名称
    static bool HasCustomName(int layer)
    {
        if (layer < 0 || layer >= MAX_LAYERS) return false;
        return !GetLayerName(layer).empty();
    }
    
    /// 获取层的显示名称（如果没有自定义名称则显示 "Layer X"）
    static std::string GetDisplayName(int layer)
    {
        if (layer < 0 || layer >= MAX_LAYERS) return "Invalid";
        std::string name = GetLayerName(layer);
        if (name.empty())
        {
            return "Layer " + std::to_string(layer);
        }
        return name;
    }
    
    /// 获取所有已命名的层（用于 UI 显示）
    static std::vector<std::pair<int, std::string>> GetAllNamedLayers()
    {
        std::vector<std::pair<int, std::string>> result;
        auto& ps = ProjectSettings::GetInstance();
        const auto& layers = ps.GetLayers();
        for (const auto& [idx, name] : layers)
        {
            result.emplace_back(idx, name);
        }
        // 按索引排序
        std::sort(result.begin(), result.end(), 
            [](const auto& a, const auto& b) { return a.first < b.first; });
        return result;
    }
};

#endif
