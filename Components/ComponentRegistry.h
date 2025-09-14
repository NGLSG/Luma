/**
 * @file ComponentRegistry.h
 * @brief 定义了组件注册相关的结构体和类，用于管理和注册游戏中的各种组件。
 */

#ifndef COMPONENTREGISTRY_H
#define COMPONENTREGISTRY_H

#include <any>
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Utils/LazySingleton.h"
#include "Utils/InspectorUI.h"
#include "IComponent.h"
#include "Event/EventBus.h"
#include "Event/Events.h"

/**
 * @brief 表示一个组件属性的注册信息。
 * 包含获取、设置属性值以及在UI中绘制属性的方法。
 */
struct PropertyRegistration
{
    std::string name; ///< 属性的名称。

    std::function<std::any(entt::registry&, entt::entity)> get; ///< 获取属性值的函数。
    std::function<void(entt::registry&, entt::entity, const std::any&)> set; ///< 设置属性值的函数。
    std::function<bool(const std::string& label, entt::registry&, entt::entity, const UIDrawData&)> draw_ui;
    ///< 在UI中绘制属性的函数。
    std::function<void(entt::registry&, entt::entity, void*)> set_from_raw_ptr; /// < 从原始指针设置属性值的函数。
    std::function<void(entt::registry&, entt::entity, void*)> get_to_raw_ptr; /// < 获取属性值到原始指针的函数。
    bool isExposedInEditor = true; ///< 指示该属性是否在编辑器中暴露。
};

/**
 * @brief 表示一个组件的注册信息。
 * 包含组件的添加、移除、检查、序列化、反序列化、获取原始指针、克隆等操作。
 */
struct ComponentRegistration
{
    std::function<void(entt::registry&, entt::entity)> add; ///< 添加组件到实体的函数。
    std::function<void(entt::registry&, entt::entity)> remove; ///< 从实体移除组件的函数。
    std::function<bool(const entt::registry&, entt::entity)> has; ///< 检查实体是否拥有该组件的函数。
    std::function<void(entt::registry&, entt::entity, const YAML::Node&)> deserialize; ///< 从YAML节点反序列化组件的函数。
    std::function<YAML::Node(const entt::registry&, entt::entity)> serialize; ///< 将组件序列化为YAML节点的函数。
    std::function<void*(entt::registry&, entt::entity)> get_raw_ptr; ///< 获取组件原始指针的函数。
    std::function<void(const entt::registry&, entt::entity, entt::registry&, entt::entity)> clone; ///< 克隆组件的函数。
    std::unordered_map<std::string, PropertyRegistration> properties; ///< 该组件的所有属性注册信息。
    size_t size = 0; ///< 组件的内存大小。
    bool isExposedInEditor = true; ///< 指示该组件是否在编辑器中暴露。
    bool isRemovable = true; ///< 指示该组件是否可以被移除。
};

/**
 * @brief 组件注册中心，用于管理所有已注册的组件类型。
 * 这是一个单例类，通过LazySingleton模式实现。
 */
class ComponentRegistry : public LazySingleton<ComponentRegistry>
{
public:
    friend class LazySingleton<ComponentRegistry>; ///< 允许LazySingleton访问私有构造函数。

    /**
     * @brief 注册一个组件类型及其相关操作。
     * @param name 组件的名称。
     * @param registration 包含组件操作函数的注册信息。
     */
    void Register(const std::string& name, ComponentRegistration registration);

    /**
     * @brief 根据名称获取已注册的组件信息。
     * @param name 组件的名称。
     * @return 指向ComponentRegistration的常量指针，如果未找到则返回nullptr。
     */
    const ComponentRegistration* Get(const std::string& name) const;

    /**
     * @brief 获取所有已注册组件的名称列表。
     * @return 包含所有已注册组件名称的字符串向量。
     */
    std::vector<std::string> GetAllRegisteredNames() const;

    /**
     * @brief 克隆源实体上的所有组件到目标实体。
     * @param sourceRegistry 源注册表。
     * @param sourceEntity 源实体。
     * @param targetRegistry 目标注册表。
     * @param targetEntity 目标实体。
     */
    void CloneAllComponents(const entt::registry& sourceRegistry, entt::entity sourceEntity,
                            entt::registry& targetRegistry, entt::entity targetEntity) const;

    /**
     * @brief 克隆源实体上的指定组件到目标实体。
     * @param componentName 要克隆的组件名称。
     * @param sourceRegistry 源注册表。
     * @param sourceEntity 源实体。
     * @param targetRegistry 目标注册表。
     * @param targetEntity 目标实体。
     * @return 如果组件成功克隆则返回true，否则返回false。
     */
    bool CloneComponent(const std::string& componentName,
                        const entt::registry& sourceRegistry, entt::entity sourceEntity,
                        entt::registry& targetRegistry, entt::entity targetEntity) const;

private:
    /**
     * @brief 默认构造函数，私有化以强制单例模式。
     */
    ComponentRegistry() = default;
    /**
     * @brief 默认析构函数。
     */
    ~ComponentRegistry() override = default;

    std::unordered_map<std::string, ComponentRegistration> m_registry; ///< 存储所有已注册组件的映射表。
};

/**
 * @brief 泛型组件注册器，用于简化特定类型组件的注册过程。
 * @tparam T 要注册的组件类型。
 */
template <typename T>
class Registry_
{
public:
    /**
     * @brief 构造函数，初始化组件的注册信息。
     * @param name 组件的名称。
     */
    explicit Registry_(std::string name)
        : m_name(std::move(name))
    {
        m_registration.add = [](entt::registry& registry, entt::entity entity)
        {
            registry.emplace<T>(entity);

            EventBus::GetInstance().Publish(ComponentAddedEvent{registry, entity, typeid(T).name()});
        };


        m_registration.remove = [](entt::registry& registry, entt::entity entity)
        {
            EventBus::GetInstance().Publish(ComponentRemovedEvent{registry, entity, typeid(T).name()});
            registry.remove<T>(entity);
        };

        m_registration.has = [](const entt::registry& reg, entt::entity e) { return reg.all_of<T>(e); };
        m_registration.deserialize = [](entt::registry& reg, entt::entity e, const YAML::Node& node)
        {
            reg.emplace_or_replace<T>(e, node.as<T>());
        };
        m_registration.serialize = [](const entt::registry& reg, entt::entity e)
        {
            return YAML::Node(reg.get<T>(e));
        };
        m_registration.get_raw_ptr = [](entt::registry& reg, entt::entity e) -> void*
        {
            return &reg.get<T>(e);
        };
        m_registration.clone = [](const entt::registry& sourceReg, entt::entity sourceEntity,
                                  entt::registry& targetReg, entt::entity targetEntity)
        {
            if (sourceReg.all_of<T>(sourceEntity))
            {
                const T& sourceComponent = sourceReg.get<T>(sourceEntity);

                targetReg.emplace_or_replace<T>(targetEntity, sourceComponent);
            }
        };
        m_registration.size = sizeof(T);
    }

    /**
     * @brief 设置组件在编辑器中不暴露。
     * @return 对当前Registry_对象的引用，支持链式调用。
     */
    Registry_& SetHidden()
    {
        m_registration.isExposedInEditor = false;
        return *this;
    }


    /**
     * @brief 析构函数，将组件注册信息提交到ComponentRegistry。
     */
    ~Registry_()
    {
        ComponentRegistry::GetInstance().Register(m_name, std::move(m_registration));
    }

    /**
     * @brief 设置组件为不可移除。
     * @return 对当前Registry_对象的引用，支持链式调用。
     */
    Registry_& SetNonRemovable()
    {
        m_registration.isRemovable = false;
        return *this;
    }

    /**
     * @brief 注册组件的一个成员变量作为属性。
     * @tparam Member 成员变量的类型。
     * @param name 属性的名称。
     * @param member_ptr 指向成员变量的指针。
     * @param isExposedInEditor 指示该属性是否在编辑器中暴露。
     * @return 对当前Registry_对象的引用，支持链式调用。
     */
    template <typename Member>
    Registry_& property(const std::string& name, Member member_ptr, bool isExposedInEditor = true)
    {
        PropertyRegistration prop;
        prop.name = name;
        prop.isExposedInEditor = isExposedInEditor;

        prop.get = [member_ptr](entt::registry& reg, entt::entity e) -> std::any
        {
            return reg.get<T>(e).*member_ptr;
        };

        prop.set = [member_ptr](entt::registry& reg, entt::entity e, const std::any& value)
        {
            using MemberType = typename std::remove_reference<decltype(std::declval<T>().*member_ptr)>::type;
            if (const auto* typed_value = std::any_cast<MemberType>(&value))
            {
                (reg.get<T>(e).*member_ptr) = *typed_value;
            }
        };
        prop.draw_ui = [member_ptr](const std::string& label, entt::registry& reg, entt::entity e,
                                    const UIDrawData& callbacks)
        {
            auto& component = reg.get<T>(e);
            auto& property_ref = component.*member_ptr;
            using MemberType = std::remove_reference_t<decltype(property_ref)>;

            if (CustomDrawing::WidgetDrawer<MemberType>::Draw(label, property_ref, callbacks))
            {
                EventBus::GetInstance().Publish(ComponentUpdatedEvent{reg, e});


                if (callbacks.onValueChanged)
                {
                    callbacks.onValueChanged.Invoke();
                }
                return true;
            }
            return false;
        };
        prop.set_from_raw_ptr = [member_ptr](entt::registry& reg, entt::entity e, void* value_ptr)
        {
            // 这个 lambda 捕获了成员的类型信息，所以在这里是类型安全的
            using MemberType = typename std::remove_reference<decltype(std::declval<T>().*member_ptr)>::type;
            if (value_ptr)
            {
                (reg.get<T>(e).*member_ptr) = *static_cast<MemberType*>(value_ptr);
            }
        };

        prop.get_to_raw_ptr = [member_ptr](entt::registry& reg, entt::entity e, void* value_ptr)
        {
            using MemberType = typename std::remove_reference<decltype(std::declval<T>().*member_ptr)>::type;
            if (value_ptr)
            {
                *static_cast<MemberType*>(value_ptr) = (reg.get<T>(e).*member_ptr);
            }
        };
        m_registration.properties[name] = std::move(prop);
        return *this;
    }

    /**
     * @brief 注册组件的一个属性，通过getter和setter函数。
     * @tparam Getter 获取属性值的函数类型。
     * @tparam Setter 设置属性值的函数类型。
     * @param name 属性的名称。
     * @param get_fn 获取属性值的函数。
     * @param set_fn 设置属性值的函数。
     * @param isExposedInEditor 指示该属性是否在编辑器中暴露。
     * @return 对当前Registry_对象的引用，支持链式调用。
     */
    template <typename Getter, typename Setter>
    Registry_& property(const std::string& name, Getter get_fn, Setter set_fn, bool isExposedInEditor = true)
    {
        PropertyRegistration prop;
        prop.name = name;
        prop.isExposedInEditor = isExposedInEditor;


        prop.get = [get_fn](entt::registry& reg, entt::entity e) -> std::any
        {
            return get_fn(reg.get<T>(e));
        };


        prop.set = [set_fn](entt::registry& reg, entt::entity e, const std::any& value)
        {
            using MemberType = std::invoke_result_t<Getter, const T&>;
            if (const auto* typed_value = std::any_cast<MemberType>(&value))
            {
                set_fn(reg.get<T>(e), *typed_value);
            }
        };
        prop.draw_ui = [get_fn, set_fn](const std::string& label, entt::registry& reg, entt::entity e,
                                        const UIDrawData& callbacks)
        {
            auto& component = reg.get<T>(e);


            using MemberType = std::invoke_result_t<Getter, const T&>;


            MemberType currentValue = get_fn(component);


            if (CustomDrawing::WidgetDrawer<MemberType>::Draw(label, currentValue, callbacks))
            {
                EventBus::GetInstance().Publish(ComponentUpdatedEvent{reg, e});
                callbacks.onValueChanged();
                set_fn(component, currentValue);
                return true;
            }
            return false;
        };
        prop.set_from_raw_ptr = [set_fn](entt::registry& reg, entt::entity e, void* value_ptr)
        {
            using MemberType = std::invoke_result_t<Getter, const T&>;
            if (value_ptr)
            {
                set_fn(reg.get<T>(e), *static_cast<MemberType*>(value_ptr));
            }
        };
        prop.get_to_raw_ptr = [get_fn](entt::registry& reg, entt::entity e, void* value_ptr)
        {
            using MemberType = std::invoke_result_t<Getter, const T&>;
            if (value_ptr)
            {
                *static_cast<MemberType*>(value_ptr) = get_fn(reg.get<T>(e));
            }
        };
        m_registration.properties[name] = std::move(prop);
        return *this;
    }

private:
    std::string m_name; ///< 组件的名称。
    ComponentRegistration m_registration; ///< 组件的注册信息。
};

/**
 * @brief 用于生成匿名变量名的宏。
 * @param str 变量名前缀。
 */
#define ANONYMOUS_VARIABLE(str) ANONYMOUS_VARIABLE_IMPL(str, __LINE__)
/**
 * @brief ANONYMOUS_VARIABLE的内部实现宏，用于拼接行号。
 * @param str 变量名前缀。
 * @param line 当前行号。
 */
#define ANONYMOUS_VARIABLE_IMPL(str, line) ANONYMOUS_VARIABLE_IMPL2(str, line)
/**
 * @brief ANONYMOUS_VARIABLE_IMPL的内部实现宏，用于实际的字符串拼接。
 * @param str 变量名前缀。
 * @param line 当前行号。
 */
#define ANONYMOUS_VARIABLE_IMPL2(str, line) str##line

/**
 * @brief 用于在全局范围注册组件的宏。
 * 它创建一个静态函数和一个静态对象，确保在程序启动时自动调用注册函数。
 */
#define REGISTRY \
static void ANONYMOUS_VARIABLE(LumaRegisterTypes)(); \
namespace { \
struct ANONYMOUS_VARIABLE(RegistryExecutor) { \
ANONYMOUS_VARIABLE(RegistryExecutor)() { ANONYMOUS_VARIABLE(LumaRegisterTypes)(); } \
}; \
static ANONYMOUS_VARIABLE(RegistryExecutor) ANONYMOUS_VARIABLE(executor_instance); \
} \
static void ANONYMOUS_VARIABLE(LumaRegisterTypes)()

#endif
