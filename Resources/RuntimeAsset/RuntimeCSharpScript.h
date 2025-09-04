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
                        ScriptClassMetadata metadata);
    ~RuntimeCSharpScript() override;

    /**
     * @brief 获取C#脚本的类名。
     * @return C#脚本的类名。
     */
    const std::string& GetScriptClassName() const;

    /**
     * @brief 获取C#脚本所属的程序集名称。
     * @return C#脚本所属的程序集名称。
     */
    const std::string& GetAssemblyName() const;

    /**
     * @brief 获取C#脚本的元数据。
     * @return C#脚本的元数据。
     */
    ScriptClassMetadata& GetMetadata();

    bool NeedsMetadataRefresh() const;
    void SetNeedsMetadataRefresh(bool value);

private:
    std::string m_className; ///< C#脚本的类名。
    std::string m_assemblyName; ///< C#脚本所属的程序集名称。
    ScriptClassMetadata m_metadata; ///< C#脚本的元数据。
    ListenerHandle m_onScriptCompliedHandle; ///< 脚本编译完成事件的监听句柄。
    bool needsMetadataRefresh = false; ///< 指示是否需要刷新元数据。
};

#endif
