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
    void SubmitFrame(std::vector<Renderable>&& frameData);
    const std::vector<RenderPacket>& GetInterpolationData();
    RenderableManager();
    void SetExternalAlpha(float a) { m_externalAlpha.store(a, std::memory_order_relaxed); }

    struct ViewportBounds
    {
        float minX, minY, maxX, maxY;
        bool valid = false;
    };
    void SetViewport(float camX, float camY, float viewW, float viewH, float zoom)
    {
        float halfW = (viewW / zoom) * 0.5f;
        float halfH = (viewH / zoom) * 0.5f;
        constexpr float kMargin = 150.0f;
        ViewportBounds vb;
        vb.minX = camX - halfW - kMargin;
        vb.minY = camY - halfH - kMargin;
        vb.maxX = camX + halfW + kMargin;
        vb.maxY = camY + halfH + kMargin;
        vb.valid = true;
        std::lock_guard<std::mutex> lock(m_viewportMutex);
        m_viewport = vb;
    }
    ViewportBounds GetViewport() const
    {
        std::lock_guard<std::mutex> lock(m_viewportMutex);
        return m_viewport;
    }
private:
    std::shared_ptr<RenderableFrame> prevFrame;
    std::shared_ptr<RenderableFrame> currFrame;
    std::mutex frameDataMutex;
    std::array<std::unique_ptr<FrameArena<RenderableTransform>>, 2> transformArenas = {
        std::make_unique<FrameArena<RenderableTransform>>(100000),
        std::make_unique<FrameArena<RenderableTransform>>(100000)
    };
    std::array<std::unique_ptr<FrameArena<std::string>>, 2> textArenas = {
        std::make_unique<FrameArena<std::string>>(4096),
        std::make_unique<FrameArena<std::string>>(4096)
    };
    std::array<std::vector<RenderPacket>, 2> packetBuffers;
    std::atomic<int> activeBufferIndex{0};
    std::atomic<std::chrono::steady_clock::time_point> lastBuiltPrevTime;
    std::atomic<std::chrono::steady_clock::time_point> lastBuiltCurrTime;
    std::atomic<uint64_t> lastBuiltPrevFrameVersion{0};
    std::atomic<uint64_t> lastBuiltCurrFrameVersion{0};
    std::unordered_map<FastSpriteBatchKey, size_t> spriteGroupIndices;
    std::unordered_map<FastTextBatchKey, size_t> textGroupIndices;
    std::vector<SceneRenderer::BatchGroup> spriteBatchGroups;
    std::vector<SceneRenderer::BatchGroup> textBatchGroups;
    std::atomic<std::chrono::steady_clock::time_point> prevStateTime;
    std::atomic<std::chrono::steady_clock::time_point> currStateTime;
    std::atomic<uint64_t> prevFrameVersion{0};
    std::atomic<uint64_t> currFrameVersion{0};
    std::atomic<float> m_externalAlpha{-1.0f};
    mutable std::mutex m_viewportMutex;
    ViewportBounds m_viewport;
    bool needsRebuild() const;
    void updateCacheState();
};
#endif 
