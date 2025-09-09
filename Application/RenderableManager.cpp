#include "RenderableManager.h"
#include <unordered_map>
#include <algorithm>
#include <variant>

#include "RenderComponent.h"
#include "Profiler.h"

void RenderableManager::SwapBuffers()
{
    std::scoped_lock lk(mutex);
    buffers[0].swap(buffers[1]);
    
    buffers[1].swap(buffers[2]);
    buffers[2].clear();
}

void RenderableManager::AddRenderable(const Renderable& renderable)
{
    std::scoped_lock lk(mutex);
    buffers[2].push_back(renderable);
}

void RenderableManager::AddRenderables(std::vector<Renderable>&& renderables)
{
    std::scoped_lock lk(mutex);
    
    buffers[2].swap(renderables);
}


const std::vector<RenderPacket>& RenderableManager::GetInterpolationData(float alpha)
{
    PROFILE_SCOPE("RenderableManager::GetInterpolationData - Total");

    std::scoped_lock lk(mutex);

    
    m_renderPackets.clear();
    m_transformArena->Reverse();
    m_textArena->Reverse();
    m_spriteGroupIndices.clear();
    m_textGroupIndices.clear();
    m_spriteBatchGroups.clear();
    m_textBatchGroups.clear();

    const auto& prevFrameRenderables = buffers[0]; 
    const auto& currFrameRenderables = buffers[1]; 

    
    PROFILE_SCOPE("RenderableManager::GetInterpolationData - Build Lookup");
    std::unordered_map<entt::entity, const ECS::TransformComponent*> prevTransforms;
    prevTransforms.reserve(prevFrameRenderables.size());
    for (const auto& renderable : prevFrameRenderables)
    {
        prevTransforms[renderable.entityId] = &renderable.transform;
    }

    
    PROFILE_SCOPE("RenderableManager::GetInterpolationData - Interpolate & Batch");
    for (const auto& currentRenderable : currFrameRenderables)
    {
        ECS::TransformComponent interpolatedTransform = currentRenderable.transform;

        
        auto it = prevTransforms.find(currentRenderable.entityId);
        if (it != prevTransforms.end())
        {
            const auto& prevTransform = *it->second;
            const auto& currentTransform = currentRenderable.transform;

            interpolatedTransform.position = ECS::Lerp(prevTransform.position, currentTransform.position, alpha);
            interpolatedTransform.scale = ECS::Lerp(prevTransform.scale, currentTransform.scale, alpha);
            interpolatedTransform.rotation = Lerp(prevTransform.rotation, currentTransform.rotation, alpha);
        }
        

        
        std::visit([&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, SpriteRenderData>)
            {
                
                const SpriteRenderData& spriteData = arg;
                FastSpriteBatchKey key(
                    spriteData.image,
                    spriteData.material,
                    spriteData.color,
                    static_cast<ECS::FilterQuality>(spriteData.filterQuality),
                    static_cast<ECS::WrapMode>(spriteData.wrapMode)
                );

                size_t groupIndex;
                auto keyIt = m_spriteGroupIndices.find(key);
                if (keyIt == m_spriteGroupIndices.end())
                {
                    groupIndex = m_spriteBatchGroups.size();
                    m_spriteGroupIndices[key] = groupIndex;
                    m_spriteBatchGroups.emplace_back();

                    auto& group = m_spriteBatchGroups.back();
                    group.image = const_cast<SkImage*>(spriteData.image);
                    group.material = spriteData.material;
                    group.sourceRect = spriteData.sourceRect;
                    group.color = spriteData.color;
                    group.zIndex = currentRenderable.zIndex;
                    group.filterQuality = spriteData.filterQuality;
                    group.wrapMode = spriteData.wrapMode;
                    group.ppuScaleFactor = spriteData.ppuScaleFactor;
                }
                else
                {
                    groupIndex = keyIt->second;
                }

                auto& group = m_spriteBatchGroups[groupIndex];
                group.transforms.emplace_back(RenderableTransform(
                    interpolatedTransform.position,
                    interpolatedTransform.scale.x,
                    interpolatedTransform.scale.y,
                    sinf(interpolatedTransform.rotation),
                    cosf(interpolatedTransform.rotation)
                ));
            }
            else if constexpr (std::is_same_v<T, TextRenderData>)
            {
                
                const TextRenderData& textData = arg;
                FastTextBatchKey key(
                    const_cast<SkTypeface*>(textData.typeface),
                    textData.fontSize,
                    static_cast<TextAlignment>(textData.alignment),
                    textData.color
                );

                size_t groupIndex;
                auto keyIt = m_textGroupIndices.find(key);
                if (keyIt == m_textGroupIndices.end())
                {
                    groupIndex = m_textBatchGroups.size();
                    m_textGroupIndices[key] = groupIndex;
                    m_textBatchGroups.emplace_back();

                    auto& group = m_textBatchGroups.back();
                    group.typeface = const_cast<SkTypeface*>(textData.typeface);
                    group.fontSize = textData.fontSize;
                    group.color = textData.color;
                    group.alignment = static_cast<TextAlignment>(textData.alignment);
                    group.zIndex = currentRenderable.zIndex;
                }
                else
                {
                    groupIndex = keyIt->second;
                }

                auto& group = m_textBatchGroups[groupIndex];
                group.transforms.emplace_back(RenderableTransform(
                    interpolatedTransform.position,
                    interpolatedTransform.scale.x,
                    interpolatedTransform.scale.y,
                    sinf(interpolatedTransform.rotation),
                    cosf(interpolatedTransform.rotation)
                ));
                group.texts.emplace_back(textData.text);
            }
        }, currentRenderable.data);
    }

    
    PROFILE_SCOPE("RenderableManager::GetInterpolationData - Packing");
    m_renderPackets.reserve(m_spriteBatchGroups.size() + m_textBatchGroups.size());

    for (const auto& group : m_spriteBatchGroups)
    {
        const size_t count = group.transforms.size();
        if (count == 0) continue;

        RenderableTransform* transformBuffer = m_transformArena->Allocate(count);
        std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

        m_renderPackets.emplace_back(RenderPacket{
            .zIndex = group.zIndex,
            .batchData = SpriteBatch{
                .material = group.material,
                .image = sk_ref_sp(group.image),
                .sourceRect = group.sourceRect,
                .color = group.color,
                .transforms = transformBuffer,
                .filterQuality = group.filterQuality,
                .wrapMode = group.wrapMode,
                .ppuScaleFactor = group.ppuScaleFactor,
                .count = count
            }
        });
    }

    for (const auto& group : m_textBatchGroups)
    {
        const size_t count = group.transforms.size();
        if (count == 0) continue;

        RenderableTransform* transformBuffer = m_transformArena->Allocate(count);
        std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

        std::string* textBuffer = m_textArena->Allocate(count);
        for (size_t i = 0; i < count; ++i)
        {
            new(&textBuffer[i]) std::string(group.texts[i]);
        }

        m_renderPackets.emplace_back(RenderPacket{
            .zIndex = group.zIndex,
            .batchData = TextBatch{
                .typeface = sk_ref_sp(group.typeface),
                .fontSize = group.fontSize,
                .color = group.color,
                .texts = textBuffer,
                .alignment = static_cast<int>(group.alignment),
                .transforms = transformBuffer,
                .count = count
            }
        });
    }

    
    PROFILE_SCOPE("RenderableManager::GetInterpolationData - Sorting");
    std::ranges::sort(m_renderPackets, [](const RenderPacket& a, const RenderPacket& b)
    {
        return a.zIndex < b.zIndex;
    });

    return m_renderPackets;
}
