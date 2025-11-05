#pragma once
#include "Utils/LazySingleton.h"
#include "Resources/AssetMetadata.h"
#include "Utils/InspectorUI.h" // 包含 CustomDrawing::WidgetDrawer
#include <any>
#include <functional>
#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include "ComponentRegistry.h"
#include "Utils/Logger.h" // 包含 LogInfo

/**
 * @brief 资产属性注册信息。
 *
 * 存储单个资产属性的元数据和
 * 类型擦除的UI绘制、Get/Set函数。
 */
struct AssetPropertyRegistration
{
    ///< 属性名称。
    std::string name;

    ///< 从YAML::Node获取属性值 (内部调用 settings.as<T>())。
    std::function<std::any(const YAML::Node& settings)> get;

    ///< 将属性值设置回YAML::Node (内部调用 settings.as<T>())。
    std::function<void(YAML::Node& settings, const std::any& value)> set;

    ///< 绘制属性UI的函数，直接操作 void* (T*) 指针。
    std::function<bool(const std::string& label, void* settings)> draw_ui;

    ///< 指示属性是否在编辑器检视器中显示。
    bool isExposedInEditor = true;
};


/**
 * @brief 资产导入器注册信息。
 *
 * 存储特定资产类型（如 AssetType::Texture）的所有属性注册
 * 以及用于支持高性能UI绘制的类型擦除辅助函数。
 */
struct AssetImporterRegistration
{
    ///< 资产属性的映射表，键为属性名称。
    std::unordered_map<std::string, AssetPropertyRegistration> properties;

    // 以下是支持 void* draw_ui 所必需的类型擦除辅助函数

    /**
     * @brief 将 YAML::Node 反序列化为 std::any 包装的C++对象。
     */
    std::function<std::any(const YAML::Node&)> Deserialize;

    /**
     * @brief 将 std::any 包装的C++对象序列化为 YAML::Node。
     */
    std::function<YAML::Node(std::any&)> Serialize;

    /**
     * @brief 从 std::any 获取指向内部C++对象的 void* 指针。
     */
    std::function<void*(std::any&)> GetDataPointer;
};


/**
 * @brief 资产导入器注册表，采用懒汉式单例模式。
 *
 * 存储所有资产类型 (AssetType) 到其导入器设置 (AssetImporterRegistration) 的映射。
 */
class AssetImporterRegistry : public LazySingleton<AssetImporterRegistry>
{
public:
    friend class LazySingleton<AssetImporterRegistry>;

    /**
     * @brief 注册一个资产导入器。
     * @param type 资产类型。
     * @param registration 资产导入器注册信息。
     */
    void Register(AssetType type, AssetImporterRegistration registration)
    {
        // 简单地将注册信息插入到映射表中
        m_registry[type] = std::move(registration);
    }

    /**
     * @brief 获取指定资产类型的导入器注册信息。
     * @param type 资产类型。
     * @return 指向资产导入器注册信息的常量指针，如果未找到则为 `nullptr`。
     */
    const AssetImporterRegistration* Get(AssetType type) const
    {
        auto it = m_registry.find(type);
        if (it != m_registry.end())
        {
            return &it->second;
        }
        return nullptr;
    }

private:
    AssetImporterRegistry() = default;
    ~AssetImporterRegistry() override = default;

    ///< 存储资产类型到导入器注册信息的映射表。
    std::unordered_map<AssetType, AssetImporterRegistration> m_registry;
};


/**
 * @brief 泛型资产注册器，用于注册特定类型 `T` 的资产属性。
 * @tparam T 要注册属性的资产类型。
 */
template <typename T>
class AssetRegistry_
{
public:
    /**
     * @brief 构造函数，初始化资产类型和类型擦除辅助函数。
     * @param type 资产类型。
     */
    explicit AssetRegistry_(AssetType type) : m_type(type)
    {
        //  填充高性能UI所必需的类型擦除辅助函数

        m_registration.Deserialize = [](const YAML::Node& node) -> std::any
        {
            return node.as<T>();
        };

        m_registration.Serialize = [](std::any& data) -> YAML::Node
        {
            T& typed_data = std::any_cast<T&>(data);
            return YAML::convert<T>::encode(typed_data);
        };

        m_registration.GetDataPointer = [](std::any& data) -> void*
        {
            return &std::any_cast<T&>(data);
        };
    }

    /**
     * @brief 析构函数，将注册信息提交到全局资产导入器注册表。
     */
    ~AssetRegistry_()
    {
        AssetImporterRegistry::GetInstance().Register(m_type, std::move(m_registration));
    }

    /**
     * @brief 注册一个资产属性。
     * @tparam Member 成员指针的类型。
     * @param name 属性的名称。
     * @param member_ptr 指向资产类型 `T` 成员的指针。
     * @param isExposedInEditor 指示属性是否在编辑器中暴露。
     * @return 对当前 `AssetRegistry_` 实例的引用，支持链式调用。
     */
    template <typename Member>
    AssetRegistry_& property(const std::string& name, Member member_ptr, bool isExposedInEditor = true)
    {
        using MemberType = std::remove_reference_t<decltype(std::declval<T>().*member_ptr)>;

        AssetPropertyRegistration prop;
        prop.name = name;
        prop.isExposedInEditor = isExposedInEditor;

        prop.get = [member_ptr](const YAML::Node& settings) -> std::any
        {
            T data = settings.as<T>();
            return data.*member_ptr;
        };


        prop.set = [member_ptr](YAML::Node& settings, const std::any& value)
        {
            if (const auto* typed_value = std::any_cast<MemberType>(&value))
            {
                T data = settings.as<T>();
                data.*member_ptr = *typed_value;
                settings = data;
            }
        };


        prop.draw_ui = [member_ptr](const std::string& label, void* settings) -> bool
        {
            T* data = static_cast<T*>(settings);

            auto& property_ref = data->*member_ptr;

            if (CustomDrawing::WidgetDrawer<MemberType>::Draw(label, property_ref, {}))
            {
                LogInfo("Asset property '{}' changed", label);
                return true;
            }
            return false;
        };

        m_registration.properties[name] = std::move(prop);
        return *this;
    }

private:
    ///< 当前注册器所关联的资产类型。
    AssetType m_type;

    ///< 存储当前资产类型的所有属性注册信息。
    AssetImporterRegistration m_registration;
};
