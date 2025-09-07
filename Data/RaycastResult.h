#ifndef RAYCASTRESULT_H
#define RAYCASTRESULT_H
#include <entt/entt.hpp>
#include "../Components/Core.h"

/**
 * @brief 表示光线投射操作的结果。
 *
 * 包含光线投射命中的实体、命中点、表面法线以及命中距离的归一化分数。
 */
struct RayCastResult
{
    entt::entity entity; ///< 光线投射命中的实体。
    ECS::Vector2f point; ///< 光线投射命中的世界坐标点。
    ECS::Vector2f normal; ///< 命中点处的表面法线。
    float fraction; ///< 光线投射距离的归一化分数（0.0到1.0）。
};

struct RayCastResults
{
    std::vector<RayCastResult> results; ///< 光线投射命中的所有结果。
};

#endif
