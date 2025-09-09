#ifndef LUMAENGINE_RENDERDATAPROVIDER_H
#define LUMAENGINE_RENDERDATAPROVIDER_H

#include <mutex>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Renderable.h"
#include "SceneRenderer.h" // 包含以获取 FrameArena 和批处理键的定义

namespace ECS
{
    inline Vector2f Lerp(const Vector2f& a, const Vector2f& b, float t)
    {
        return a + (b - a) * t;
    }
}

inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

class RenderableManager : public LazySingleton<RenderableManager>
{
public:
    friend class LazySingleton<RenderableManager>;

    void SwapBuffers();

    void AddRenderable(const Renderable& renderable);

    void AddRenderables(std::vector<Renderable>&& renderables);

    const std::vector<RenderPacket>& GetInterpolationData(float alpha);

private:
    std::array<std::vector<Renderable>, 3> buffers;
    std::mutex mutex;

    std::unique_ptr<FrameArena<RenderableTransform>> m_transformArena = std::make_unique<FrameArena<RenderableTransform>>(100000);
    std::unique_ptr<FrameArena<std::string>> m_textArena = std::make_unique<FrameArena<std::string>>(4096);

    std::unordered_map<FastSpriteBatchKey, size_t> m_spriteGroupIndices;
    std::unordered_map<FastTextBatchKey, size_t> m_textGroupIndices;

    std::vector<SceneRenderer::BatchGroup> m_spriteBatchGroups;
    std::vector<SceneRenderer::BatchGroup> m_textBatchGroups;

    std::vector<RenderPacket> m_renderPackets;
};


#endif //LUMAENGINE_RENDERDATAPROVIDER_H