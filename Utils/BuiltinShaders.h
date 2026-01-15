#ifndef LUMAENGINE_BUILTINSHADERS_H
#define LUMAENGINE_BUILTINSHADERS_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Guid.h"
#include "../Data/ShaderData.h"

/**
 * @brief 内建Shader信息
 */
struct BuiltinShaderInfo
{
    std::string name;           ///< 显示名称
    std::string guidStr;        ///< 固定GUID字符串
    std::string moduleName;     ///< 对应的shader文件名
};

/**
 * @brief 内建Shader管理器
 * 提供内建shader的GUID映射和查询功能
 * 
 * 内建shader是完整的shader文件（没有export语句），可直接用于材质渲染
 * 有export语句的文件是模块，供用户在自己的shader中import使用
 */
class BuiltinShaders
{
public:
    /**
     * @brief 获取所有内建shader列表
     */
    static const std::vector<BuiltinShaderInfo>& GetAllBuiltinShaders();

    /**
     * @brief 检查GUID是否为内建Shader的GUID
     */
    static bool IsBuiltinShaderGuid(const Guid& guid);

    /**
     * @brief 获取内建Shader的显示名称
     * @return 如果是内建shader返回名称，否则返回空字符串
     */
    static std::string GetBuiltinShaderName(const Guid& guid);

    /**
     * @brief 获取内建Shader的模块名称
     * @return 如果是内建shader返回模块名称，否则返回空字符串
     */
    static std::string GetBuiltinShaderModuleName(const Guid& guid);

    /**
     * @brief 根据GUID获取内建Shader的ShaderData
     * @return 如果是内建shader返回对应的ShaderData，否则返回空的ShaderData
     */
    static Data::ShaderData GetBuiltinShaderData(const Guid& guid);

private:
    /**
     * @brief 内建Shader列表
     * 只包含完整的shader（没有export语句，可直接用于材质渲染）
     * 使用固定GUID格式: 00000000-0000-0000-0000-00000000000X
     */
    static const std::vector<BuiltinShaderInfo> s_builtinShaders;
};

#endif //LUMAENGINE_BUILTINSHADERS_H
