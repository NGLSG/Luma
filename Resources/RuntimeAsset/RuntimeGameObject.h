#ifndef RUNTIMEGAMEOBJECT_H
#define RUNTIMEGAMEOBJECT_H

#include <entt/entt.hpp>
#include <string>
#include <vector> // Added for std::vector
#include <unordered_map> // Added for std::unordered_map

#include "IRuntimeAsset.h"

/// @brief 组件注册信息结构体的前向声明。
struct ComponentRegistration;
/// @brief 运行时场景类的前向声明。
class RuntimeScene;

namespace Data
{
    /// @brief 预制体节点数据结构。
    struct PrefabNode;
}

/**
 * @brief 运行时游戏对象类，封装了entt实体和场景上下文。
 *
 * 代表游戏中的一个实体，可以拥有组件、父子关系和各种属性。
 */
class RuntimeGameObject
{
public:
    /// @brief 默认构造函数。
    RuntimeGameObject() = default;
    /**
     * @brief 使用entt实体句柄和运行时场景构造一个RuntimeGameObject。
     * @param handle entt实体句柄。
     * @param scene 实体所属的运行时场景指针。
     */
    RuntimeGameObject(entt::entity handle, RuntimeScene* scene);
    /// @brief 拷贝构造函数。
    RuntimeGameObject(const RuntimeGameObject& other) = default;
    /// @brief 拷贝赋值运算符。
    RuntimeGameObject& operator=(const RuntimeGameObject& other) = default;

    /**
     * @brief 为游戏对象添加一个组件。
     * @tparam T 要添加的组件类型。
     * @tparam Args 构造组件所需的参数类型。
     * @param args 构造组件所需的参数。
     * @return 对新添加组件的引用。
     */
    template <typename T, typename... Args>
    T& AddComponent(Args&&... args);

    /**
     * @brief 获取游戏对象上的一个组件。
     * @tparam T 要获取的组件类型。
     * @return 对指定类型组件的引用。
     */
    template <typename T>
    T& GetComponent();
    /**
     * @brief 检查游戏对象是否拥有指定类型的组件。
     * @tparam T 要检查的组件类型。
     * @return 如果拥有该组件则返回true，否则返回false。
     */
    template <typename T>
    bool HasComponent() const;

    /**
     * @brief 移除游戏对象上的一个组件。
     * @tparam T 要移除的组件类型。
     */
    template <typename T>
    void RemoveComponent() const;

    /**
     * @brief 设置当前游戏对象的父对象。
     * @param parent 要设置的父游戏对象。
     */
    void SetParent(RuntimeGameObject parent);
    /// @brief 将当前游戏对象设置为场景的根对象（移除父对象）。
    void SetRoot();
    /**
     * @brief 获取当前游戏对象的父对象。
     * @return 父游戏对象。如果当前对象没有父对象，则返回一个无效的RuntimeGameObject。
     */
    RuntimeGameObject GetParent();
    /**
     * @brief 获取当前游戏对象的所有子对象。
     * @return 包含所有子游戏对象的vector。
     */
    std::vector<RuntimeGameObject> GetChildren();
    /**
     * @brief 检查游戏对象是否处于激活状态。
     * @return 如果游戏对象及其所有父对象都处于激活状态，则返回true。
     */
    bool IsActive();
    /**
     * @brief 设置游戏对象的激活状态。
     * @param active 要设置的激活状态（true为激活，false为非激活）。
     */
    void SetActive(bool active);

    /**
     * @brief 获取游戏对象的名称。
     * @return 游戏对象的名称字符串。
     */
    std::string GetName();
    /**
     * @brief 设置游戏对象的名称。
     * @param name 要设置的名称字符串。
     */
    void SetName(const std::string& name);
    /**
     * @brief 获取游戏对象的全局唯一标识符（GUID）。
     * @return 游戏对象的GUID。
     */
    Guid GetGuid();

    /**
     * @brief 隐式转换为entt实体句柄。
     * @return 对应的entt实体句柄。
     */
    operator entt::entity() const { return m_entityHandle; }
    /**
     * @brief 比较两个RuntimeGameObject是否相等。
     * @param other 另一个RuntimeGameObject对象。
     * @return 如果两个对象的实体句柄相同，则返回true，否则返回false。
     */
    bool operator==(const RuntimeGameObject& other) const;
    /**
     * @brief 检查当前游戏对象是否有效。
     * @return 如果实体句柄有效且关联的场景存在，则返回true。
     */
    bool IsValid();

    /**
     * @brief 将当前游戏对象序列化为预制体数据节点。
     * @return 包含游戏对象数据的PrefabNode结构体。
     */
    Data::PrefabNode SerializeToPrefabData();
    /**
     * @brief 检查当前游戏对象是否是另一个游戏对象的后代。
     * @param dragged_object 要检查的潜在祖先游戏对象。
     * @return 如果当前对象是`dragged_object`的后代，则返回true。
     */
    bool IsDescendantOf(RuntimeGameObject dragged_object);
    /**
     * @brief 设置游戏对象的子对象列表。
     * @param children 包含新子对象的vector。
     */
    void SetChildren(std::vector<RuntimeGameObject>& children);

    /**
     * @brief 设置游戏对象在其兄弟列表中的索引位置。
     * @param newIndex 新的兄弟索引。
     */
    void SetSiblingIndex(int newIndex);
    /**
     * @brief 获取游戏对象在其兄弟列表中的索引位置。
     * @return 兄弟索引。
     */
    int GetSiblingIndex();
    /**
     * @brief 获取游戏对象底层的entt实体句柄。
     * @return entt实体句柄。
     */
    entt::entity GetEntityHandle() const { return m_entityHandle; }
    /**
     * @brief 获取游戏对象上所有组件的映射。
     * @return 包含组件名称到ComponentRegistration指针的unordered_map。
     */
    std::unordered_map<std::string, const ComponentRegistration*> GetAllComponents();

private:
    entt::entity m_entityHandle; ///< 关联的entt实体句柄。
    std::vector<std::string> m_componentNames; ///< 游戏对象上组件的名称列表。
    RuntimeScene* m_scene; ///< 实体所属的运行时场景指针。
};


#endif