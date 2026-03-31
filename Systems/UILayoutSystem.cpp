#include "UILayoutSystem.h"
#include "Components/UIComponents.h"
#include "Components/RelationshipComponent.h"
#include "Resources/RuntimeAsset/RuntimeScene.h"
#include <algorithm>
#include <vector>

namespace Systems
{
    void UILayoutSystem::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
    }

    void UILayoutSystem::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<ECS::UILayoutComponent>();
        for (auto entity : view)
        {
            LayoutEntity(registry, entity);
        }
    }

    void UILayoutSystem::LayoutEntity(entt::registry& registry, entt::entity entity)
    {
        auto& layout = registry.get<ECS::UILayoutComponent>(entity);
        if (!layout.isVisible || !layout.Enable)
            return;

        auto* children = registry.try_get<ECS::ChildrenComponent>(entity);
        if (!children || children->children.empty())
            return;

        struct ChildInfo
        {
            entt::entity entity;
            ECS::IUIComponent* ui;
            float mainSize;
            float crossSize;
        };

        std::vector<ChildInfo> childInfos;
        childInfos.reserve(children->children.size());

        bool isHorizontal = (layout.direction == ECS::LayoutDirection::Horizontal);

        for (auto child : children->children)
        {
            if (!registry.valid(child))
                continue;

            ECS::IUIComponent* childUI = nullptr;

            if (auto* c = registry.try_get<ECS::UILayoutComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::ButtonComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::InputTextComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::ToggleButtonComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::RadioButtonComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::CheckBoxComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::SliderComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::ComboBoxComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::ExpanderComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::ProgressBarComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::TabControlComponent>(child))
                childUI = c;
            else if (auto* c = registry.try_get<ECS::ListBoxComponent>(child))
                childUI = c;

            if (!childUI || !childUI->isVisible)
                continue;

            float w = childUI->rect.z;
            float h = childUI->rect.w;
            childInfos.push_back({child, childUI,
                                  isHorizontal ? w : h,
                                  isHorizontal ? h : w});
        }

        if (childInfos.empty())
            return;

        float padLeft = layout.padding.x;
        float padTop = layout.padding.y;
        float padRight = layout.padding.z;
        float padBottom = layout.padding.w;

        float containerMainSize = isHorizontal
                                      ? (layout.rect.z - padLeft - padRight)
                                      : (layout.rect.w - padTop - padBottom);
        float containerCrossSize = isHorizontal
                                       ? (layout.rect.w - padTop - padBottom)
                                       : (layout.rect.z - padLeft - padRight);

        struct Line
        {
            size_t startIdx;
            size_t count;
            float totalMain;
            float maxCross;
        };

        std::vector<Line> lines;
        {
            size_t i = 0;
            while (i < childInfos.size())
            {
                Line line{i, 0, 0.0f, 0.0f};
                while (i < childInfos.size())
                {
                    float needed = line.totalMain + childInfos[i].mainSize;
                    if (line.count > 0)
                        needed += layout.spacing;

                    if (layout.wrap == ECS::LayoutWrap::Wrap && line.count > 0 && needed > containerMainSize)
                        break;

                    line.totalMain += childInfos[i].mainSize;
                    if (line.count > 0)
                        line.totalMain += layout.spacing;
                    line.maxCross = std::max(line.maxCross, childInfos[i].crossSize);
                    line.count++;
                    i++;
                }
                lines.push_back(line);
            }
        }

        float totalCross = 0.0f;
        for (auto& l : lines)
            totalCross += l.maxCross;
        if (lines.size() > 1)
            totalCross += layout.spacing * static_cast<float>(lines.size() - 1);

        if (layout.autoSize)
        {
            if (isHorizontal)
            {
                float maxMain = 0.0f;
                for (auto& l : lines)
                    maxMain = std::max(maxMain, l.totalMain);
                layout.rect.z = maxMain + padLeft + padRight;
                layout.rect.w = totalCross + padTop + padBottom;
            }
            else
            {
                float maxMain = 0.0f;
                for (auto& l : lines)
                    maxMain = std::max(maxMain, l.totalMain);
                layout.rect.w = maxMain + padTop + padBottom;
                layout.rect.z = totalCross + padLeft + padRight;
            }
            containerMainSize = isHorizontal
                                    ? (layout.rect.z - padLeft - padRight)
                                    : (layout.rect.w - padTop - padBottom);
            containerCrossSize = isHorizontal
                                     ? (layout.rect.w - padTop - padBottom)
                                     : (layout.rect.z - padLeft - padRight);
        }

        float crossOffset = 0.0f;

        for (auto& line : lines)
        {
            float mainOffset = 0.0f;
            float extraSpacing = 0.0f;

            float freeMain = containerMainSize - line.totalMain;
            if (freeMain < 0) freeMain = 0;

            switch (layout.mainAlign)
            {
            case ECS::LayoutAlign::Start:
                mainOffset = 0.0f;
                break;
            case ECS::LayoutAlign::Center:
                mainOffset = freeMain * 0.5f;
                break;
            case ECS::LayoutAlign::End:
                mainOffset = freeMain;
                break;
            case ECS::LayoutAlign::SpaceBetween:
                if (line.count > 1)
                    extraSpacing = freeMain / static_cast<float>(line.count - 1);
                break;
            case ECS::LayoutAlign::SpaceAround:
                if (line.count > 0)
                {
                    float gap = freeMain / static_cast<float>(line.count);
                    mainOffset = gap * 0.5f;
                    extraSpacing = gap;
                }
                break;
            }

            float cursor = mainOffset;

            for (size_t j = 0; j < line.count; j++)
            {
                auto& ci = childInfos[line.startIdx + j];

                float crossPos = crossOffset;
                float crossExtra = line.maxCross - ci.crossSize;
                switch (layout.crossAlign)
                {
                case ECS::LayoutAlign::Start:
                    break;
                case ECS::LayoutAlign::Center:
                    crossPos += crossExtra * 0.5f;
                    break;
                case ECS::LayoutAlign::End:
                    crossPos += crossExtra;
                    break;
                default:
                    break;
                }

                if (isHorizontal)
                {
                    ci.ui->rect.x = padLeft + cursor;
                    ci.ui->rect.y = padTop + crossPos;
                }
                else
                {
                    ci.ui->rect.x = padLeft + crossPos;
                    ci.ui->rect.y = padTop + cursor;
                }

                cursor += ci.mainSize + layout.spacing + extraSpacing;
            }

            crossOffset += line.maxCross + layout.spacing;
        }
    }
}
