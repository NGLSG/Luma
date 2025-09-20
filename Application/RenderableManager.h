#ifndef LUMAENGINE_RENDERABLEMANAGER_H
#define LUMAENGINE_RENDERABLEMANAGER_H

#include <mutex>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Renderable.h"
#include "SceneRenderer.h"


class RenderableManager : public LazySingleton<RenderableManager>
{
public:
    friend class LazySingleton<RenderableManager>;
    void SubmitFrame(std::vector<Renderable>&& frameData);
    const std::vector<RenderPacket>& GetInterpolationData();

    RenderableManager();

private:
    std::shared_ptr<std::vector<Renderable>> m_prevFrame;
    std::shared_ptr<std::vector<Renderable>> m_currFrame;
    std::mutex mutex;

    std::array<std::unique_ptr<FrameArena<RenderableTransform>>, 2> m_transformArenas = {
        std::make_unique<FrameArena<RenderableTransform>>(100000),
        std::make_unique<FrameArena<RenderableTransform>>(100000)
    };
    std::array<std::unique_ptr<FrameArena<std::string>>, 2> m_textArenas = {
        std::make_unique<FrameArena<std::string>>(4096),
        std::make_unique<FrameArena<std::string>>(4096)
    };
    std::array<std::vector<RenderPacket>, 2> m_packetBuffers;
    int m_activeBufferIndex = 0;

    std::shared_ptr<std::vector<Renderable>> m_lastBuiltPrevFrame;
    std::shared_ptr<std::vector<Renderable>> m_lastBuiltCurrFrame;
    std::chrono::steady_clock::time_point m_lastBuiltPrevTime;
    std::chrono::steady_clock::time_point m_lastBuiltCurrTime;

    std::unordered_map<FastSpriteBatchKey, size_t> m_spriteGroupIndices;
    std::unordered_map<FastTextBatchKey, size_t> m_textGroupIndices;

    std::vector<SceneRenderer::BatchGroup> m_spriteBatchGroups;
    std::vector<SceneRenderer::BatchGroup> m_textBatchGroups;

    std::chrono::steady_clock::time_point m_prevStateTime;
    std::chrono::steady_clock::time_point m_currStateTime;
};


#endif //LUMAENGINE_RENDERABLEMANAGER_H
