#include "AnimationSystem.h"

#include "AnimationControllerComponent.h"
#include "Logger.h"
#include "Loaders/AnimationControllerLoader.h"
#include "RuntimeAsset/RuntimeScene.h"

void Systems::AnimationSystem::OnCreate(RuntimeScene* scene, EngineContext& engineCtx)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<ECS::AnimationControllerComponent>();
    auto loader = AnimationControllerLoader();


    for (auto entity : view)
    {
        auto& animComp = view.get<ECS::AnimationControllerComponent>(entity);
        if (!animComp.animationController.assetGuid.Valid())
        {
            LogWarn("实体 {} 的 AnimationControllerComponent 没有设置动画控制器资源", (uint32_t)entity);
            continue;
        }


        animComp.runtimeController = loader.LoadAsset(animComp.animationController.assetGuid);

        if (!animComp.runtimeController)
        {
            LogWarn("无法加载实体 {} 的动画控制器资源: {}", (uint32_t)entity,
                    animComp.animationController.assetGuid.ToString());
            continue;
        }


        animComp.runtimeController->SetFrameRate(animComp.targetFrame);
        animComp.runtimeController->PlayEntryAnimation();
    }
}

void Systems::AnimationSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx)
{
    auto& registry = scene->GetRegistry();
    auto view = registry.view<ECS::AnimationControllerComponent>();


    std::vector<ECS::AnimationControllerComponent*> componentsToUpdate;

    for (auto entity : view)
    {
        if (!scene->FindGameObjectByEntity(entity).IsActive())
        {
            continue;
        }

        auto& animComp = view.get<ECS::AnimationControllerComponent>(entity);


        if (animComp.runtimeController && animComp.Enable)
        {
            componentsToUpdate.push_back(&animComp);
        }
    }

    const size_t numComponents = componentsToUpdate.size();
    if (numComponents == 0)
    {
        return;
    }


    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());


    const size_t batchSize = (numComponents + numThreads - 1) / numThreads;

    std::vector<std::jthread> workers;
    workers.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; ++i)
    {
        const size_t start = i * batchSize;
        const size_t end = std::min(start + batchSize, numComponents);

        if (start >= end)
        {
            continue;
        }


        workers.emplace_back([&componentsToUpdate, deltaTime, start, end]()
        {
            for (size_t j = start; j < end; ++j)
            {
                componentsToUpdate[j]->runtimeController->Update(deltaTime);
            }
        });
    }
}
