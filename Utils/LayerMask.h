#ifndef LAYERMASK_H
#define LAYERMASK_H

#include <cstdint>
#include <string>
#include "yaml-cpp/yaml.h"

/**
 * @brief 光照层掩码类型，用于控制光源与物体之间的交互
 * 
 * 类似 Unity 的 LayerMask，支持32个层（Layer 0-31）
 * 使用位掩码实现，每个位代表一个层
 */
struct LayerMask
{
    uint32_t value = 0xFFFFFFFF; // 默认所有层都启用
    
    LayerMask() = default;
    explicit LayerMask(uint32_t v) : value(v) {}
    
    /// 检查是否包含指定层
    bool Contains(int layer) const
    {
        if (layer < 0 || layer >= 32) return false;
        return (value & (1u << layer)) != 0;
    }
    
    /// 设置指定层
    void Set(int layer, bool enabled)
    {
        if (layer < 0 || layer >= 32) return;
        if (enabled)
            value |= (1u << layer);
        else
            value &= ~(1u << layer);
    }
    
    /// 检查两个 LayerMask 是否有交集
    bool Intersects(const LayerMask& other) const
    {
        return (value & other.value) != 0;
    }
    
    /// 转换为 uint32_t
    operator uint32_t() const { return value; }
    
    /// 全部启用
    static LayerMask All() { return LayerMask(0xFFFFFFFF); }
    
    /// 全部禁用（不受光照影响）
    static LayerMask None() { return LayerMask(0); }
    
    /// 只启用指定层
    static LayerMask Only(int layer)
    {
        if (layer < 0 || layer >= 32) return None();
        return LayerMask(1u << layer);
    }
};

namespace YAML
{
    template <>
    struct convert<LayerMask>
    {
        static Node encode(const LayerMask& mask)
        {
            Node node;
            node = mask.value;
            return node;
        }
        
        static bool decode(const Node& node, LayerMask& mask)
        {
            if (!node.IsScalar())
                return false;
            mask.value = node.as<uint32_t>();
            return true;
        }
    };
}

#endif
