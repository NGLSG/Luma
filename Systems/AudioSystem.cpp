#include "AudioSystem.h"
#include "../Application/AudioManager.h"
#include "../Resources/Loaders/AudioLoader.h"
#include "../Components/AudioComponent.h"
#include "../Components/Transform.h"
#include "../Utils/Logger.h"

namespace Systems
{
    void AudioSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        if (!AudioManager::GetInstance().Initialize(48000, 2))
        {
            LogError("AudioSystem: Failed to initialize AudioManager.");
            return;
        }


        auto& reg = scene->GetRegistry();
        auto view = reg.view<ECS::AudioComponent>();
        AudioLoader loader(AudioManager::GetInstance().GetSampleRate(),
                           AudioManager::GetInstance().GetChannels());

        for (auto e : view)
        {
            auto& ac = view.get<ECS::AudioComponent>(e);
            if (!ac.playOnStart || !ac.audioHandle.Valid()) continue;

            sk_sp<RuntimeAudio> ra = loader.LoadAsset(ac.audioHandle.assetGuid);
            if (!ra) continue;

            AudioManager::PlayDesc desc;
            desc.audio = ra;
            desc.loop = ac.loop;
            desc.volume = ac.volume;
            desc.spatial = ac.spatial;


            if (reg.all_of<ECS::Transform>(e))
            {
                auto& tc = reg.get<ECS::Transform>(e);
                auto p = tc.position;
                desc.sourceX = p.x;
                desc.sourceY = p.y;
                desc.sourceZ = 0;
            }

            desc.minDistance = ac.minDistance;
            desc.maxDistance = ac.maxDistance;

            ac.voiceId = AudioManager::GetInstance().Play(desc);
        }
    }

    void AudioSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& reg = scene->GetRegistry();
        auto view = reg.view<ECS::AudioComponent>();


        AudioLoader loader(AudioManager::GetInstance().GetSampleRate(),
                           AudioManager::GetInstance().GetChannels());

        for (auto e : view)
        {
            if (!scene->FindGameObjectByEntity(e).IsActive())
                continue;
            auto& ac = view.get<ECS::AudioComponent>(e);
            if (!ac.Enable)
                continue;

            if (ac.requestedPlay && ac.audioHandle.Valid())
            {
                sk_sp<RuntimeAudio> ra = loader.LoadAsset(ac.audioHandle.assetGuid);
                if (ra)
                {
                    AudioManager::PlayDesc desc;
                    desc.audio = ra;
                    desc.loop = ac.loop;
                    desc.volume = ac.volume;
                    desc.spatial = ac.spatial;

                    if (reg.all_of<ECS::Transform>(e))
                    {
                        auto& tc = reg.get<ECS::Transform>(e);
                        auto p = tc.position;
                        desc.sourceX = p.x;
                        desc.sourceY = p.y;
                        desc.sourceZ = 0;
                    }
                    desc.minDistance = ac.minDistance;
                    desc.maxDistance = ac.maxDistance;

                    ac.voiceId = AudioManager::GetInstance().Play(desc);
                }
                ac.requestedPlay = false;
            }


            if (ac.voiceId != 0 && reg.all_of<ECS::Transform>(e))
            {
                auto& tc = reg.get<ECS::Transform>(e);
                auto p = tc.position;
                AudioManager::GetInstance().SetVoicePosition(ac.voiceId, p.x, p.y, 0);
            }


            if (ac.voiceId != 0 && AudioManager::GetInstance().IsFinished(ac.voiceId))
            {
                ac.voiceId = 0;
            }
        }
    }

    void AudioSystem::OnDestroy(RuntimeScene* scene)
    {
        AudioManager::GetInstance().Shutdown();
    }
}
