#ifndef INDIRECT_LIGHTING_SYSTEM_H
#define INDIRECT_LIGHTING_SYSTEM_H

/**
 * @file IndirectLightingSystem.h
 * @brief 间接光照系统
 * 
 * 负责收集场景中的反射体信息，计算间接光照贡献。
 * 实现简化的全局光照效果，让阴影区域能够接收到反射光。
 * 
 * Feature: 2d-lighting-system
 */

#include "ISystem.h"
#include "../Components/LightingTypes.h"
#include "../Renderer/Nut/Buffer.h"
#include <vector>
#include <memory>

namespace Systems
{
    /// 最大反射体数量
    static constexpr uint32_t MAX_REFLECTORS = 64;

    /**
     * @brief 间接光照系统
     * 
     * 该系统负责：
     * - 收集场景中被光照的物体作为反射体
     * - 计算反射体的颜色和位置
     * - 更新 GPU 缓冲区供 shader 使用
     */
    class IndirectLightingSystem : public ISystem
    {
    public:
        /**
         * @brief 默认构造函数
         */
        IndirectLightingSystem();

        /**
         * @brief 析构函数
         */
        ~IndirectLightingSystem() override = default;

        /**
         * @brief 系统创建时调用
         */
        void OnCreate(RuntimeScene* scene, EngineContext& engineCtx) override;

        /**
         * @brief 系统每帧更新时调用
         */
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx) override;

        /**
         * @brief 系统销毁时调用
         */
        void OnDestroy(RuntimeScene* scene) override;

        // ==================== 数据访问 ====================

        /**
         * @brief 获取反射体数据缓冲区
         */
        std::shared_ptr<Nut::Buffer> GetReflectorBuffer() const { return m_reflectorBuffer; }

        /**
         * @brief 获取间接光照全局数据缓冲区
         */
        std::shared_ptr<Nut::Buffer> GetGlobalBuffer() const { return m_globalBuffer; }

        /**
         * @brief 获取反射体数量
         */
        uint32_t GetReflectorCount() const { return static_cast<uint32_t>(m_reflectors.size()); }

        /**
         * @brief 获取单例实例
         */
        static IndirectLightingSystem* GetInstance() { return s_instance; }

        /**
         * @brief 检查是否启用
         */
        bool IsEnabled() const { return m_enabled; }

        /**
         * @brief 设置启用状态
         */
        void SetEnabled(bool enable) { m_enabled = enable; }

    private:
        /**
         * @brief 收集场景中的反射体
         */
        void CollectReflectors(RuntimeScene* scene);

        /**
         * @brief 创建 GPU 缓冲区
         */
        void CreateBuffers(EngineContext& engineCtx);

        /**
         * @brief 更新 GPU 缓冲区
         */
        void UpdateBuffers();

        /**
         * @brief 从场景获取间接光照设置
         */
        void UpdateSettingsFromScene(RuntimeScene* scene);

    private:
        std::vector<ECS::IndirectLightData> m_reflectors;       ///< 反射体数据
        ECS::IndirectLightingGlobalData m_globalData;           ///< 全局数据

        std::shared_ptr<Nut::Buffer> m_reflectorBuffer;         ///< 反射体数据缓冲区
        std::shared_ptr<Nut::Buffer> m_globalBuffer;            ///< 全局数据缓冲区

        bool m_enabled = true;                                  ///< 是否启用
        bool m_buffersCreated = false;                          ///< 缓冲区是否已创建
        float m_indirectRadius = 200.0f;                        ///< 间接光照影响半径

        EngineContext* m_engineCtx = nullptr;                   ///< 引擎上下文缓存

        static IndirectLightingSystem* s_instance;              ///< 单例实例
    };
}

#endif // INDIRECT_LIGHTING_SYSTEM_H
