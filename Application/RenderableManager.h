#ifndef LUMAENGINE_RENDERABLEMANAGER_H
#define LUMAENGINE_RENDERABLEMANAGER_H

#include <array>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <mutex>
#include "Renderable.h"
#include "SceneRenderer.h"

class RenderableManager : public LazySingleton<RenderableManager>
{
public:
    friend class LazySingleton<RenderableManager>;

    using RenderableFrame = const std::vector<Renderable>;

    /**
     * @brief 提交新的帧数据（vector 版本）
     * @param frameData 帧渲染数据
     */
    void SubmitFrame(std::vector<Renderable>&& frameData);

    /**
     * @brief 获取插值后的渲染数据
     * @return 渲染包数组
     * @details 当其中一帧为空时，会选择正常帧作为基准，不进行插值
     */
    const std::vector<RenderPacket>& GetInterpolationData();

    RenderableManager();

    // 外部设置插值因子（优先级高于内部基于时间的估算）。
    // 传入范围应为 [0,1]。传入其他值将忽略并使用内部计算。
    void SetExternalAlpha(float a) { m_externalAlpha.store(a, std::memory_order_relaxed); }

private:
    // 帧数据存储，使用 std::shared_ptr 实现线程安全的快照
    std::shared_ptr<RenderableFrame> prevFrame;
    std::shared_ptr<RenderableFrame> currFrame;

    // 使用互斥锁保护帧数据的交换
    std::mutex frameDataMutex;

    // 双缓冲内存分配器
    std::array<std::unique_ptr<FrameArena<RenderableTransform>>, 2> transformArenas = {
        std::make_unique<FrameArena<RenderableTransform>>(100000),
        std::make_unique<FrameArena<RenderableTransform>>(100000)
    };
    std::array<std::unique_ptr<FrameArena<std::string>>, 2> textArenas = {
        std::make_unique<FrameArena<std::string>>(4096),
        std::make_unique<FrameArena<std::string>>(4096)
    };

    // 使用vector存储渲染包
    std::array<std::vector<RenderPacket>, 2> packetBuffers;
    std::atomic<int> activeBufferIndex{0};

    // 缓存检查用的时间戳（使用原子操作）
    std::atomic<std::chrono::steady_clock::time_point> lastBuiltPrevTime;
    std::atomic<std::chrono::steady_clock::time_point> lastBuiltCurrTime;
    std::atomic<uint64_t> lastBuiltPrevFrameVersion{0};
    std::atomic<uint64_t> lastBuiltCurrFrameVersion{0};

    // 批次分组索引（这些在单线程构建阶段使用，不需要线程安全）
    std::unordered_map<FastSpriteBatchKey, size_t> spriteGroupIndices;
    std::unordered_map<FastTextBatchKey, size_t> textGroupIndices;

    // 批次组存储
    std::vector<SceneRenderer::BatchGroup> spriteBatchGroups;
    std::vector<SceneRenderer::BatchGroup> textBatchGroups;

    // 时间戳（使用原子操作）
    std::atomic<std::chrono::steady_clock::time_point> prevStateTime;
    std::atomic<std::chrono::steady_clock::time_point> currStateTime;

    // 帧版本号，用于缓存检查
    std::atomic<uint64_t> prevFrameVersion{0};
    std::atomic<uint64_t> currFrameVersion{0};

    // 可选：外部驱动的插值 alpha（来自主循环 accumulator/fixed dt），默认 -1 表示未启用
    std::atomic<float> m_externalAlpha{-1.0f};

    /**
     * @brief 检查是否需要重新构建渲染数据
     * @return true 如果需要重新构建
     */
    bool needsRebuild() const;

    /**
     * @brief 更新缓存状态
     */
    void updateCacheState();
};

#endif //LUMAENGINE_RENDERABLEMANAGER_H