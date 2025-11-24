#ifndef LUMAENGINE_SHADERMODULEINITIALIZER_H
#define LUMAENGINE_SHADERMODULEINITIALIZER_H

#include <filesystem>
#include <string>
#include <vector>

namespace Nut
{
    class ShaderModuleRegistry;
    /**
     * @brief Shader模块初始化器
     * 负责自动扫描和注册引擎提供的shader模块
     */
    class ShaderModuleInitializer
    {
    public:
        /**
         * @brief 初始化引擎shader模块
         * 自动扫描Shaders目录下的所有.wgsl文件并注册
         * 
         * @param shadersPath 可选的自定义shader目录路径，如果为空则使用默认路径
         *                    - Windows/Linux: ./Shaders
         *                    - Android: PathUtils::GetAndroidInternalPath()/Shaders
         * @return 成功注册的模块数量
         */
        static int InitializeEngineShaderModules(const std::filesystem::path& shadersPath = "");

        /**
         * @brief 递归扫描目录并注册所有shader模块
         * @param directory 要扫描的目录
         * @param registeredCount 输出：已注册的模块数量
         */
        static void ScanAndRegisterShaders(const std::filesystem::path& directory, int& registeredCount);

        /**
         * @brief 递归扫描目录并注册所有shader模块（接受registry引用）
         * @param directory 要扫描的目录
         * @param registeredCount 输出：已注册的模块数量
         * @param registry 模块注册表引用
         */
        static void ScanAndRegisterShaders(const std::filesystem::path& directory, int& registeredCount, ShaderModuleRegistry& registry);

        /**
         * @brief 从shader文件内容中提取export的模块名
         * @param shaderCode shader源代码
         * @param moduleName 输出：提取的模块名
         * @return 如果找到export语句返回true
         */
        static bool ExtractModuleName(const std::string& shaderCode, std::string& moduleName);

        /**
         * @brief 从shader代码中移除export语句
         * @param shaderCode shader源代码
         * @return 移除export语句后的代码
         */
        static std::string RemoveExportStatement(const std::string& shaderCode);

        /**
         * @brief 获取默认的shader目录路径
         * @return shader目录路径
         */
        static std::filesystem::path GetDefaultShadersPath();

    private:
        ShaderModuleInitializer() = delete;
    };

} // namespace Nut

#endif //LUMAENGINE_SHADERMODULEINITIALIZER_H
