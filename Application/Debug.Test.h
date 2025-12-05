#ifndef DEBUG_TEST_H
#define DEBUG_TEST_H
class RuntimeScene;
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Components/Transform.h"
#include "../Components/Rigidbody.h"
#include "../Components/ColliderComponent.h"
#include "../Components/Sprite.h"
#include "../Resources/AssetManager.h"
#include <random>
#include "../Utils/Logger.h"
namespace Debug
{
    class SceneGenerator
    {
    public:
        static void GenerateSpriteTest(RuntimeScene* scene, int count);
    };
    void SceneGenerator::GenerateSpriteTest(RuntimeScene* scene, int count)
    {
        if (!scene) return;
        LogInfo("Generating physics performance test with {} sprites...", count);
        scene->Clear();
        float screenWidth = 1280.0f;
        float screenHeight = 720.0f;
        float wallThickness = 100.0f;
        const AssetMetadata* spriteMeta = AssetManager::GetInstance().GetMetadata("sprite.png");
        if (!spriteMeta)
        {
            LogError("SceneGenerator: Failed to find 'Assets/sprite.png'. Aborting test scene generation.");
            return;
        }
        AssetHandle spriteHandle(spriteMeta->guid);
        std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> pos_x_dist(-640, 640.0f);
        std::uniform_real_distribution<float> pos_y_dist(-360.0f, 360.0f);
        std::uniform_real_distribution<float> scale_dist(0.008f, 0.012f);
        std::uniform_real_distribution<float> rot_dist(0.0f, 2.0f * 3.14159f);
        for (int i = 0; i < count; ++i)
        {
            RuntimeGameObject go = scene->CreateGameObject("Sprite_" + std::to_string(i));
            auto& transform = go.GetComponent<ECS::TransformComponent>();
            transform.position = {pos_x_dist(gen), pos_y_dist(gen)};
            transform.rotation = rot_dist(gen);
            transform.scale = scale_dist(gen);
            auto& sprite = go.AddComponent<ECS::SpriteComponent>();
            sprite.textureHandle = spriteHandle;
            sprite.color = ECS::Colors::White;
            sprite.sourceRect = {0.0f, 0.0f, 2000.f, 2666.0f};
        }
        LogInfo("SceneGenerator: Generated {} dynamic sprite entities for performance test.", count);
    }
}
#endif
