#ifndef RUNTIMECSHARPSCRIPT_H
#define RUNTIMECSHARPSCRIPT_H

#include "IRuntimeAsset.h"
#include <string>
#include <utility>

#include "ScriptMetadata.h"

/**
 * @brief 表示一个运行时C#脚本。
 *
 * 该类封装了一个C#脚本的运行时信息，包括其类名、程序集名称和元数据。
 */
class RuntimeCSharpScript : public IRuntimeAsset
{
public:
    /**
     * @brief 构造一个新的运行时C#脚本实例。
     * @param sourceGuid 脚本的源GUID。
     * @param className C#脚本的类名。
     * @param assemblyName C#脚本所属的程序集名称。
     * @param metadata C#脚本的元数据。
     */
    RuntimeCSharpScript(const Guid& sourceGuid,
                        std::string className,
                        std::string assemblyName,
                        ScriptClassMetadata metadata)
        : m_className(std::move(className)),
          m_assemblyName(std::move(assemblyName)),
          m_metadata(std::move(metadata))
    {
        m_sourceGuid = sourceGuid;
    }

    /**
     * @brief 获取C#脚本的类名。
     * @return C#脚本的类名。
     */
    const std::string& GetScriptClassName() const { return m_className; }

    /**
     * @brief 获取C#脚本所属的程序集名称。
     * @return C#脚本所属的程序集名称。
     */
    const std::string& GetAssemblyName() const { return m_assemblyName; }

    /**
     * @brief 获取C#脚本的元数据。
     * @return C#脚本的元数据。
     */
    ScriptClassMetadata& GetMetadata() { return m_metadata; }

private:
    std::string m_className;        ///< C#脚本的类名。
    std::string m_assemblyName;     ///< C#脚本所属的程序集名称。
    ScriptClassMetadata m_metadata; ///< C#脚本的元数据。
};

#endif