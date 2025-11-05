#ifndef RUNTIMEPHYSICSMATERIAL_H
#define RUNTIMEPHYSICSMATERIAL_H

#include "IRuntimeAsset.h"
#include "box2d/types.h"

/**
 * @brief 运行时物理材质类。
 * 继承自IRuntimeAsset，表示一种可在运行时使用的物理材质资源。
 */
class RuntimePhysicsMaterial : public IRuntimeAsset
{
public:
    /**
     * @brief 构造函数，创建一个运行时物理材质实例。
     * @param sourceGuid 材质的源GUID。
     * @param friction 摩擦力系数。
     * @param restitution 恢复力/弹性系数。
     * @param rollingResistance 滚动阻力系数。
     * @param tangentSpeed 切向速度。
     */
    RuntimePhysicsMaterial(const Guid& sourceGuid, float friction, float restitution, float rollingResistance,
                           float tangentSpeed)
        : m_friction(friction), m_restitution(restitution),
          m_rollingResistance(rollingResistance), m_tangentSpeed(tangentSpeed)
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 获取摩擦力系数。
     * @return 摩擦力系数。
     */
    float GetFriction() const { return m_friction; }
    /**
     * @brief 获取恢复力/弹性系数。
     * @return 恢复力/弹性系数。
     */
    float GetRestitution() const { return m_restitution; }
    /**
     * @brief 获取滚动阻力系数。
     * @return 滚动阻力系数。
     */
    float GetRollingResistance() const { return m_rollingResistance; }
    /**
     * @brief 获取切向速度。
     * @return 切向速度。
     */
    float GetTangentSpeed() const { return m_tangentSpeed; }

    /**
     * @brief 类型转换操作符，将当前物理材质转换为Box2D的b2SurfaceMaterial类型。
     * @return 对应的b2SurfaceMaterial实例。
     */
    operator b2SurfaceMaterial() const
    {
        return ToB2SurfaceMaterial();
    }

    /**
     * @brief 将当前物理材质转换为Box2D的b2SurfaceMaterial类型。
     * @return 对应的b2SurfaceMaterial实例。
     */
    b2SurfaceMaterial ToB2SurfaceMaterial() const
    {
        b2SurfaceMaterial material;
        material.friction = m_friction;
        material.restitution = m_restitution;
        material.rollingResistance = m_rollingResistance;
        material.tangentSpeed = m_tangentSpeed;
        return material;
    }

private:
    float m_friction; ///< 摩擦力系数。
    float m_restitution; ///< 恢复力/弹性系数。
    float m_rollingResistance; ///< 滚动阻力系数。
    float m_tangentSpeed; ///< 切向速度。
};

#endif