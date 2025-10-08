#include "SystemsManager.h"
#include "../../Systems/ISystem.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"

SystemsManager::~SystemsManager()
{
    Clear();
}

void SystemsManager::InitializeSystems(RuntimeScene* scene, EngineContext& engineCtx)
{
    
    for (auto& system : m_essentialSimulationSystems)
    {
        system->OnCreate(scene, engineCtx);
    }

    
    for (auto& system : m_essentialMainThreadSystems)
    {
        system->OnCreate(scene, engineCtx);
    }

    
    for (auto& system : m_simulationSystems)
    {
        system->OnCreate(scene, engineCtx);
    }

    
    for (auto& system : m_mainThreadSystems)
    {
        system->OnCreate(scene, engineCtx);
    }
}

void SystemsManager::UpdateSimulationSystems(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx,
                                             bool pauseNormalSystems)
{
    
    for (auto& system : m_essentialSimulationSystems)
    {
        system->OnUpdate(scene, deltaTime, engineCtx);
    }

    
    if (!pauseNormalSystems)
    {
        for (auto& system : m_simulationSystems)
        {
            system->OnUpdate(scene, deltaTime, engineCtx);
        }
    }
}

void SystemsManager::UpdateMainThreadSystems(RuntimeScene* scene, float deltaTime, EngineContext& engineCtx,
                                             bool pauseNormalSystems)
{
    
    for (auto& system : m_essentialMainThreadSystems)
    {
        system->OnUpdate(scene, deltaTime, engineCtx);
    }

    
    if (!pauseNormalSystems)
    {
        for (auto& system : m_mainThreadSystems)
        {
            system->OnUpdate(scene, deltaTime, engineCtx);
        }
    }
}

void SystemsManager::DestroySystems(RuntimeScene* scene)
{
    
    for (auto it = m_mainThreadSystems.rbegin(); it != m_mainThreadSystems.rend(); ++it)
    {
        (*it)->OnDestroy(scene);
    }

    
    for (auto it = m_simulationSystems.rbegin(); it != m_simulationSystems.rend(); ++it)
    {
        (*it)->OnDestroy(scene);
    }

    
    for (auto it = m_essentialMainThreadSystems.rbegin(); it != m_essentialMainThreadSystems.rend(); ++it)
    {
        (*it)->OnDestroy(scene);
    }

    
    for (auto it = m_essentialSimulationSystems.rbegin(); it != m_essentialSimulationSystems.rend(); ++it)
    {
        (*it)->OnDestroy(scene);
    }
}

void SystemsManager::Clear()
{
    m_mainThreadSystems.clear();
    m_simulationSystems.clear();
    m_essentialMainThreadSystems.clear();
    m_essentialSimulationSystems.clear();
}
