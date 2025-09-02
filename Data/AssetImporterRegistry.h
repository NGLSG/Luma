#pragma once
#include "Utils/LazySingleton.h"
#include "Resources/AssetMetadata.h"
#include "Utils/InspectorUI.h"
#include <any>
#include <functional>
#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include "ComponentRegistry.h"


/**
 * @brief 资产属性注册信息。
 */
struct AssetPropertyRegistration
{
    std::string name; ///< 属性名称。

    std::function<std::any(const YAML::Node& settings)> get; ///< 获取属性值的函数。

    std::function<void(YAML::Node& settings, const std::any& value)> set; ///< 设置属性值的函数。

    std::function<bool(const std::string& label, YAML::Node& settings)> draw_ui; ///< 绘制属性UI的函数。
    bool isExposedInEditor = true; ///< 指示属性是否在编辑器中暴露。
};


/**
 * @brief 资产导入器注册信息。
 */
struct AssetImporterRegistration
{
    std::unordered_map<std::string, AssetPropertyRegistration> properties; ///< 资产属性的映射表，键为属性名称。
};


/**
 * @brief 资产导入器注册表，采用懒汉式单例模式。
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
    void Register(AssetType type, AssetImporterRegistration registration);

    /**
     * @brief 获取指定资产类型的导入器注册信息。
     * @param type 资产类型。
     * @return 指向资产导入器注册信息的指针，如果未找到则为 `nullptr`。
     */
    const AssetImporterRegistration* Get(AssetType type) const;

private:
    AssetImporterRegistry() = default;
    ~AssetImporterRegistry() override = default;
    std::unordered_map<AssetType, AssetImporterRegistration> m_registry; ///< 存储资产类型到导入器注册信息的映射表。
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
     * @brief 构造函数，初始化资产类型。
     * @param type 资产类型。
     */
    explicit AssetRegistry_(AssetType type) : m_type(type)
    {
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


        prop.draw_ui = [member_ptr](const std::string& label, YAML::Node& settings) -> bool
        {
            T data = settings.as<T>();
            auto& property_ref = data.*member_ptr;


            if (CustomDrawing::WidgetDrawer<MemberType>::Draw(label, property_ref, {}))
            {
                LogInfo("Asset property '{}' changed", label);
                settings = YAML::convert<T>::encode(data);
                return true;
            }
            return false;
        };

        m_registration.properties[name] = std::move(prop);
        return *this;
    }

private:
    AssetType m_type; ///< 当前注册器所关联的资产类型。
    AssetImporterRegistration m_registration; ///< 存储当前资产类型的所有属性注册信息。
};