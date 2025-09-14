#ifndef LUMAENGINE_RENDERABLEMANAGER_H
#define LUMAENGINE_RENDERABLEMANAGER_H

#include <mutex>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Renderable.h"
#include "SceneRenderer.h" // 包含以获取 FrameArena 和批处理键的定义


class RenderableManager : public LazySingleton<RenderableManager>
{
public:
    friend class LazySingleton<RenderableManager>;
    void SubmitFrame(std::vector<Renderable>&& frameData);
    const std::vector<RenderPacket>& GetInterpolationData();

    RenderableManager();

private:
    std::array<std::vector<Renderable>, 3> buffers;
    std::mutex mutex;

    std::unique_ptr<FrameArena<RenderableTransform>> m_transformArena = std::make_unique<FrameArena<
        RenderableTransform>>(100000);
    std::unique_ptr<FrameArena<std::string>> m_textArena = std::make_unique<FrameArena<std::string>>(4096);

    std::unordered_map<FastSpriteBatchKey, size_t> m_spriteGroupIndices;
    std::unordered_map<FastTextBatchKey, size_t> m_textGroupIndices;

    std::vector<SceneRenderer::BatchGroup> m_spriteBatchGroups;
    std::vector<SceneRenderer::BatchGroup> m_textBatchGroups;

    std::vector<RenderPacket> m_renderPackets;
    std::chrono::steady_clock::time_point m_prevStateTime;
    std::chrono::steady_clock::time_point m_currStateTime;
};


#endif //LUMAENGINE_RENDERABLEMANAGER_H
