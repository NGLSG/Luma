#ifndef LUMAENGINE_SHADERMODULEREGISTRY_H
#define LUMAENGINE_SHADERMODULEREGISTRY_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <set>
#include "../../Utils/LazySingleton.h"

namespace Nut
{
    /**
     * @brief Shader模块信息
     * 存储单个shader模块的源代码和元数据
     */
    struct ShaderModuleInfo
    {
        std::string moduleName;        ///< 完整模块名 (e.g., "Common.Math")
        std::string sourceCode;        ///< 模块源代码
        std::vector<std::string> dependencies; ///< 依赖的模块列表
        
        ShaderModuleInfo() = default;
        ShaderModuleInfo(const std::string& name, const std::string& code)
            : moduleName(name), sourceCode(code) {}
    };

    /**
     * @brief Shader模块注册表
     * 管理所有shader模块的内存注册表，支持分层模块名称
     * 
     * 模块命名规则：
     * - 使用点号分隔的层级结构：FatherModule.ChildModule.xxx
     * - 父模块会自动包含所有子模块
     * 
     * 语法规则：
     * - "import ModuleName;" - 导入模块
     * - "export ModuleName;" - 声明当前文件为模块并导出
     */
    class ShaderModuleRegistry: public LazySingleton<ShaderModuleRegistry>
    {
        friend class LazySingleton<ShaderModuleRegistry>;
        
    public:
        /**
         * @brief 注册一个shader模块
         * @param moduleName 模块名称（支持点号分隔的层级）
         * @param sourceCode 模块源代码
         */
        void RegisterModule(const std::string& moduleName, const std::string& sourceCode);

        /**
         * @brief 获取模块信息
         * @param moduleName 模块名称
         * @return 模块信息指针，如果不存在返回nullptr
         */
        const ShaderModuleInfo* GetModule(const std::string& moduleName) const;

        /**
         * @brief 检查模块是否存在
         * @param moduleName 模块名称
         * @return 如果模块存在返回true
         */
        bool HasModule(const std::string& moduleName) const;

        /**
         * @brief 获取所有子模块
         * @param parentModuleName 父模块名称
         * @return 所有子模块的名称列表
         */
        std::vector<std::string> GetChildModules(const std::string& parentModuleName) const;

        /**
         * @brief 清空所有注册的模块
         */
        void Clear();

        /**
         * @brief 获取所有模块名称
         * @return 所有已注册模块的名称列表
         */
        std::vector<std::string> GetAllModuleNames() const;

    private:
        ShaderModuleRegistry();
        ~ShaderModuleRegistry() = default;
        ShaderModuleRegistry(const ShaderModuleRegistry&) = delete;
        ShaderModuleRegistry& operator=(const ShaderModuleRegistry&) = delete;

        std::unordered_map<std::string, ShaderModuleInfo> m_modules;
        
        /**
         * @brief 初始化引擎shader模块
         * 在构造函数中自动调用
         */
        void InitializeEngineModules();
    };

    /**
     * @brief Shader模块展开器
     * 负责解析import/export语句并递归展开模块引用
     */
    class ShaderModuleExpander
    {
    public:
        /**
         * @brief 展开shader代码中的所有模块引用
         * @param sourceCode 原始shader代码
         * @param errorMessage 如果展开失败，存储错误信息
         * @param exportedModuleName 输出：如果代码包含export语句，返回导出的模块名
         * @return 展开后的完整shader代码，失败返回空字符串
         */
        static std::string ExpandModules(const std::string& sourceCode, std::string& errorMessage, std::string& exportedModuleName);

    private:
        /**
         * @brief 解析import语句
         * @param line 代码行
         * @param moduleName 输出：解析出的模块名
         * @return 如果是有效的import语句返回true
         */
        static bool ParseImportStatement(const std::string& line, std::string& moduleName);

        /**
         * @brief 解析export语句
         * @param line 代码行
         * @param moduleName 输出：解析出的模块名
         * @return 如果是有效的export语句返回true
         */
        static bool ParseExportStatement(const std::string& line, std::string& moduleName);

        /**
         * @brief 解析模块名称，支持::语法
         * @param moduleName 原始模块名（可能包含::）
         * @param currentContext 当前模块上下文
         * @return 解析后的完整模块名
         */
        static std::string ResolveModuleName(const std::string& moduleName, const std::string& currentContext);

        /**
         * @brief 递归收集所有依赖模块
         * @param moduleName 模块名称
         * @param allModules 输出：所有依赖的模块集合
         * @param moduleStack 模块栈（用于循环检测）
         * @param errorMessage 错误信息
         * @param currentContext 当前模块上下文
         * @return 成功返回true
         */
        static bool CollectAllDependencies(
            const std::string& moduleName,
            std::set<std::string>& allModules,
            std::vector<std::string>& moduleStack,
            std::string& errorMessage,
            const std::string& currentContext);

        /**
         * @brief 对模块进行拓扑排序
         * @param modules 要排序的模块集合
         * @param errorMessage 错误信息
         * @return 排序后的模块列表（入度少的在前）
         */
        static std::vector<std::string> TopologicalSort(
            const std::set<std::string>& modules,
            std::string& errorMessage);

        /**
         * @brief 递归展开单个模块
         * @param moduleName 模块名称
         * @param expandedModules 已展开的模块集合（用于循环检测）
         * @param moduleStack 模块栈（用于错误报告）
         * @param output 输出字符串流
         * @param errorMessage 错误信息
         * @return 成功返回true
         */
        static bool ExpandModuleRecursive(
            const std::string& moduleName,
            std::set<std::string>& expandedModules,
            std::vector<std::string>& moduleStack,
            std::ostringstream& output,
            std::string& errorMessage);

        /**
         * @brief 展开模块及其所有子模块
         * @param moduleName 模块名称
         * @param expandedModules 已展开的模块集合
         * @param moduleStack 模块栈
         * @param output 输出字符串流
         * @param errorMessage 错误信息
         * @return 成功返回true
         */
        static bool ExpandModuleWithChildren(
            const std::string& moduleName,
            std::set<std::string>& expandedModules,
            std::vector<std::string>& moduleStack,
            std::ostringstream& output,
            std::string& errorMessage);
    };

} // namespace Nut

#endif //LUMAENGINE_SHADERMODULEREGISTRY_H
