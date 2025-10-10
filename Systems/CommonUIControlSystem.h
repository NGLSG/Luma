#ifndef COMMON_UI_CONTROL_SYSTEM_H
#define COMMON_UI_CONTROL_SYSTEM_H

#include "ISystem.h"
#include <entt/entt.hpp>

namespace Systems
{
    /**
     * @brief 管理 ToggleButton、RadioButton、CheckBox、Slider、ComboBox、Expander、
     * ProgressBar、TabControl 和 ListBox 等常见 UI 组件的运行时行为。
     */
    class CommonUIControlSystem final : public ISystem
    {
    public:
        void OnCreate(RuntimeScene* scene, EngineContext& context) override;
        void OnDestroy(RuntimeScene* scene) override;
        void OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context) override;

    private:
        entt::entity m_activeSlider = entt::null;
        entt::entity m_openCombo = entt::null;

        static float Clamp01(float value);
    };
}

#endif // COMMON_UI_CONTROL_SYSTEM_H

