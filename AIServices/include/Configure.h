/**
 * @file configure.h
 * @brief 包含各种模型和系统配置的结构体定义，以及 YAML 序列化/反序列化转换器。
 */
#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <yaml-cpp/yaml.h>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

/**
 * @brief LLama 模型创建信息结构体。
 */
struct LLamaCreateInfo
{
    std::string model = ""; ///< 模型名称。
    int contextSize = 32000; ///< 上下文大小。
    int maxTokens = 4096; ///< 最大令牌数。
    int requirePermission = 0; ///< 所需权限等级。
};

/**
 * @brief Claude API 创建信息结构体。
 */
struct ClaudeAPICreateInfo
{
    bool enable = false; ///< 是否启用。
    std::string apiKey; ///< API 密钥。
    std::string model = "claude-3.5"; ///< 模型名称。
    std::string apiVersion = "2023-06-01"; ///< API 版本。
    std::string _endPoint = "https://api.anthropic.com/v1/complete"; ///< API 端点。
    std::vector<std::string> supportedModels; ///< 支持的模型列表。
    int requirePermission = 0; ///< 所需权限等级。
};

/**
 * @brief OpenAI 机器人创建信息结构体。
 */
struct OpenAIBotCreateInfo
{
    bool enable = true; ///< 是否启用。
    bool useWebProxy = false; ///< 是否使用网页代理。
    std::string api_key = ""; ///< API 密钥。
    std::string model = "gpt-4o"; ///< 模型名称。
    std::string proxy = ""; ///< 代理地址。
    std::string _endPoint = ""; ///< API 端点。
    std::vector<std::string> supportedModels; ///< 支持的模型列表。
    int requirePermission = 0; ///< 所需权限等级。
};

/**
 * @brief 类似 GPT 的模型创建信息结构体。
 */
struct GPTLikeCreateInfo
{
    bool enable; ///< 是否启用。
    bool useLocalModel = false; ///< 是否使用本地模型。
    std::string api_key; ///< API 密钥。
    std::string model = ""; ///< 模型名称。
    std::string apiHost = ""; ///< API 主机地址。
    std::string apiPath = ""; ///< API 路径。
    LLamaCreateInfo llamaData; ///< LLama 模型创建信息。
    std::vector<std::string> supportedModels; ///< 支持的模型列表。
    int requirePermission = 0; ///< 所需权限等级。

    /**
     * @brief 默认构造函数，初始化 enable 为 false。
     */
    GPTLikeCreateInfo()
    {
        enable = false;
    }

    /**
     * @brief 赋值运算符重载。
     * @param data 源 GPTLikeCreateInfo 对象。
     * @return 对当前对象的引用。
     */
    GPTLikeCreateInfo& operator=(const GPTLikeCreateInfo& data)
    {
        this->enable = data.enable;
        this->api_key = data.api_key;
        this->model = data.model;
        this->apiHost = data.apiHost;
        this->apiPath = data.apiPath;
        this->llamaData = data.llamaData;
        this->useLocalModel = data.useLocalModel;
        this->supportedModels = data.supportedModels;
        this->requirePermission = data.requirePermission;

        return *this;
    }
};

/**
 * @brief Claude 机器人创建信息结构体。
 */
struct ClaudeBotCreateInfo
{
    bool enable = false; ///< 是否启用。
    std::string channelID; ///< 频道ID。
    std::string slackToken; ///< Slack 令牌。
    std::string userName; ///< 用户名。
    std::string cookies; ///< Cookies。
    std::vector<std::string> supportedModels; ///< 支持的模型列表。
    int requirePermission = 0; ///< 所需权限等级。
};

/**
 * @brief Gemini 机器人创建信息结构体。
 */
struct GeminiBotCreateInfo
{
    bool enable = false; ///< 是否启用。
    std::string _apiKey; ///< API 密钥。
    std::string _endPoint; ///< API 端点。
    std::string model = "gemini-2.0-flash"; ///< 模型名称。
    std::vector<std::string> supportedModels; ///< 支持的模型列表。
    int requirePermission = 0; ///< 所需权限等级。
};

/**
 * @brief 响应角色配置结构体。
 */
struct ResponseRole
{
    std::string suffix = ""; ///< 响应数据后缀。
    std::string content = ""; ///< 内容路径。
    std::string callback = ""; ///< 回调函数名。
    std::string stopFlag = "[DONE]"; ///< 停止标志。
};

/**
 * @brief API 密钥角色配置结构体。
 */
struct APIKeyRole
{
    std::string key = ""; ///< API 密钥字段名。
    std::string role = "HEADERS"; ///< 密钥在请求中的角色 (例如 HEADERS)。
    std::string header = "Authorization: Bearer "; ///< 完整的请求头前缀。
};

/**
 * @brief 参数角色配置结构体。
 */
struct ParamsRole
{
    std::string suffix = "messages"; ///< 参数后缀。
    std::string path = "content"; ///< 参数路径。
    std::string content = "content"; ///< 参数内容。
    bool isStr = false; ///< 内容是否为字符串类型。
};

/**
 * @brief 提示词角色配置结构体。
 */
struct PromptRole
{
    ParamsRole role; ///< 角色参数配置。
    ParamsRole prompt; ///< 提示词参数配置。
};

/**
 * @brief 自定义变量结构体。
 */
struct CustomVariable
{
    std::string name = ""; ///< 变量名称。
    bool isStr = false; ///< 变量值是否为字符串类型。
    std::string value = ""; ///< 变量值。
};

/**
 * @brief 自定义规则结构体。
 */
struct CustomRule
{
    bool enable = false; ///< 是否启用此自定义规则。
    bool supportSystemRole = false; ///< 是否支持系统角色。
    std::string author = "Ryoshi"; ///< 规则作者。
    std::string version = "1.0"; ///< 规则版本。
    std::string description = "自定义规则"; ///< 规则描述。
    std::string name = ""; ///< 规则名称。
    std::string model = ""; ///< 模型名称。
    std::string apiPath =
        "https://generativelanguage.googleapis.com/v1beta/models/${MODEL}:streamGenerateContent?key=${API_KEY}"; ///< API 路径。
    std::vector<CustomVariable> vars; ///< 自定义变量列表。
    APIKeyRole apiKeyRole; ///< API 密钥角色配置。

    PromptRole promptRole; ///< 提示词角色配置。
    std::vector<ParamsRole> params; ///< 参数列表。
    std::vector<ParamsRole> extraMust; ///< 额外必需参数列表。
    std::unordered_map<std::string, std::string> headers; ///< 自定义请求头。
    std::unordered_map<std::string, std::string> roles{{"system", ""}, {"user", ""}, {"assistant", ""}}; ///< 角色映射 (system, user, assistant)。
    ResponseRole responseRole{"data: ", "choices/delta/content", "RESPONSE", "[DONE"}; ///< 响应角色配置。
    std::vector<std::string> supportedModels; ///< 支持的模型列表。
    int requirePermission = 0; ///< 所需权限等级。
};

/**
 * @brief 用户信息结构体。
 */
struct UserInfo
{
    std::string nickName = ""; ///< 昵称。
    std::string email = ""; ///< 邮箱。
    std::string passwd = ""; ///< 密码。
    int permissions = 0; ///< 权限等级。
    std::vector<std::string> authKeys; ///< 授权密钥列表。
    float money = 0; ///< 金额。
};

/**
 * @brief 全局配置结构体。
 */
struct Configure
{
    bool enableAuth = false; ///< 是否启用认证。
    UserInfo admin = {"admin", "null", "123456", 999}; ///< 管理员用户信息。
    OpenAIBotCreateInfo openAi; ///< OpenAI 机器人配置。
    ClaudeBotCreateInfo claude; ///< Claude 机器人配置。
    GeminiBotCreateInfo gemini; ///< Gemini 机器人配置。
    GPTLikeCreateInfo grok; ///< Grok 机器人配置。
    GPTLikeCreateInfo mistral; ///< Mistral 机器人配置。
    GPTLikeCreateInfo qianwen; ///< 千问机器人配置。
    GPTLikeCreateInfo sparkdesk; ///< 星火大模型机器人配置。
    GPTLikeCreateInfo chatglm; ///< ChatGLM 机器人配置。
    GPTLikeCreateInfo hunyuan; ///< 混元大模型机器人配置。
    GPTLikeCreateInfo baichuan; ///< 百川大模型机器人配置。
    GPTLikeCreateInfo huoshan; ///< 火山大模型机器人配置。
    ClaudeAPICreateInfo claudeAPI; ///< Claude API 配置。
    std::unordered_map<std::string, GPTLikeCreateInfo> customGPTs; ///< 自定义 GPT 配置映射。
    std::vector<CustomRule> customRules; ///< 自定义规则列表。
};

namespace YAML
{
    /**
     * @brief YAML 转换器，用于序列化和反序列化 ResponseRole 类型。
     */
    template <>
    struct convert<ResponseRole>
    {
        /**
         * @brief 从 YAML 节点解码数据到 ResponseRole 类型。
         * @param node YAML 节点。
         * @param rhs 目标 ResponseRole 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ResponseRole& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }

            if (node["suffix"])
                rhs.suffix = node["suffix"].as<std::string>();
            if (node["content"])
                rhs.content = node["content"].as<std::string>();
            if (node["callback"])
                rhs.callback = node["callback"].as<std::string>();
            if (node["stopFlag"])
                rhs.stopFlag = node["stopFlag"].as<std::string>();

            return true;
        }

        /**
         * @brief 将 ResponseRole 类型数据编码为 YAML 节点。
         * @param rhs 源 ResponseRole 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const ResponseRole& rhs)
        {
            Node node;

            node["suffix"] = rhs.suffix;
            node["content"] = rhs.content;
            node["callback"] = rhs.callback;
            node["stopFlag"] = rhs.stopFlag;
            return node;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 APIKeyRole 类型。
     */
    template <>
    struct convert<APIKeyRole>
    {
        /**
         * @brief 从 YAML 节点解码数据到 APIKeyRole 类型。
         * @param node YAML 节点。
         * @param rhs 目标 APIKeyRole 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, APIKeyRole& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }

            if (node["key"])
                rhs.key = node["key"].as<std::string>();
            if (node["role"])
                rhs.role = node["role"].as<std::string>();
            if (node["header"])
                rhs.header = node["header"].as<std::string>();

            return true;
        }

        /**
         * @brief 将 APIKeyRole 类型数据编码为 YAML 节点。
         * @param rhs 源 APIKeyRole 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const APIKeyRole& rhs)
        {
            Node node;
            node["key"] = rhs.key;
            node["role"] = rhs.role;
            node["header"] = rhs.header;
            return node;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ParamsRole 类型。
     */
    template <>
    struct convert<ParamsRole>
    {
        /**
         * @brief 从 YAML 节点解码数据到 ParamsRole 类型。
         * @param node YAML 节点。
         * @param rhs 目标 ParamsRole 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ParamsRole& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }

            if (node["suffix"])
                rhs.suffix = node["suffix"].as<std::string>();
            if (node["path"])
                rhs.path = node["path"].as<std::string>();
            if (node["content"])
                rhs.content = node["content"].as<std::string>();
            if (node["isStr"])
                rhs.isStr = node["isStr"].as<bool>();

            return true;
        }

        /**
         * @brief 将 ParamsRole 类型数据编码为 YAML 节点。
         * @param rhs 源 ParamsRole 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const ParamsRole& rhs)
        {
            Node node;
            node["suffix"] = rhs.suffix;
            node["path"] = rhs.path;
            node["content"] = rhs.content;
            node["isStr"] = rhs.isStr;
            return node;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 PromptRole 类型。
     */
    template <>
    struct convert<PromptRole>
    {
        /**
         * @brief 从 YAML 节点解码数据到 PromptRole 类型。
         * @param node YAML 节点。
         * @param rhs 目标 PromptRole 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, PromptRole& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }

            if (node["role"])
                rhs.role = node["role"].as<ParamsRole>();
            if (node["prompt"])
                rhs.prompt = node["prompt"].as<ParamsRole>();

            return true;
        }

        /**
         * @brief 将 PromptRole 类型数据编码为 YAML 节点。
         * @param rhs 源 PromptRole 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const PromptRole& rhs)
        {
            Node node;
            node["role"] = rhs.role;
            node["prompt"] = rhs.prompt;
            return node;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 CustomVariable 类型。
     */
    template <>
    struct convert<CustomVariable>
    {
        /**
         * @brief 从 YAML 节点解码数据到 CustomVariable 类型。
         * @param node YAML 节点。
         * @param rhs 目标 CustomVariable 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, CustomVariable& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }
            if (node["name"])
                rhs.name = node["name"].as<std::string>();
            if (node["value"])
                rhs.value = node["value"].as<std::string>();
            if (node["isStr"])
                rhs.isStr = node["isStr"].as<bool>();
            return true;
        }

        /**
         * @brief 将 CustomVariable 类型数据编码为 YAML 节点。
         * @param rhs 源 CustomVariable 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const CustomVariable& rhs)
        {
            Node node;
            node["name"] = rhs.name;
            node["value"] = rhs.value;
            node["isStr"] = rhs.isStr;
            return node;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 CustomRule 类型。
     */
    template <>
    struct convert<CustomRule>
    {
        /**
         * @brief 从 YAML 节点解码数据到 CustomRule 类型。
         * @param node YAML 节点。
         * @param rhs 目标 CustomRule 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, CustomRule& rhs)
        {
            if (!node.IsMap())
            {
                return false;
            }

            if (node["enable"])
                rhs.enable = node["enable"].as<bool>();
            if (node["supportSystemRole"])
                rhs.supportSystemRole = node["supportSystemRole"].as<bool>();
            if (node["name"])
                rhs.name = node["name"].as<std::string>();
            if (node["model"])
                rhs.model = node["model"].as<std::string>();
            if (node["apiPath"])
                rhs.apiPath = node["apiPath"].as<std::string>();

            if (node["apiKeyRole"])
                rhs.apiKeyRole = node["apiKeyRole"].as<APIKeyRole>();

            if (node["headers"] && node["headers"].IsMap())
            {
                for (const auto& header : node["headers"])
                {
                    rhs.headers[header.first.as<std::string>()] = header.second.as<std::string>();
                }
            }

            if (node["roles"] && node["roles"].IsMap())
            {
                for (const auto& role : node["roles"])
                {
                    rhs.roles[role.first.as<std::string>()] = role.second.as<std::string>();
                }
            }
            if (node["supportModels"] && node["supportModels"].IsSequence())
            {
                rhs.supportedModels.clear();
                for (const auto& model : node["supportModels"])
                {
                    rhs.supportedModels.push_back(model.as<std::string>());
                }
            }

            if (node["promptRole"])
                rhs.promptRole = node["promptRole"].as<PromptRole>();

            if (node["params"] && node["params"].IsSequence())
            {
                rhs.params.clear();
                for (const auto& param : node["params"])
                {
                    rhs.params.push_back(param.as<ParamsRole>());
                }
            }

            if (node["responseRole"])
                rhs.responseRole = node["responseRole"].as<ResponseRole>();
            if (node["author"])
                rhs.author = node["author"].as<std::string>();
            if (node["version"])
                rhs.version = node["version"].as<std::string>();
            if (node["description"])
                rhs.description = node["description"].as<std::string>();
            if (node["vars"])
                for (const auto& var : node["vars"])
                {
                    rhs.vars.push_back(var.as<CustomVariable>());
                }
            if (node["extraMust"])
            {
                int i = 0;
                rhs.extraMust.clear();
                for (const auto& must : node["extraMust"])
                {
                    rhs.extraMust.emplace_back(must.as<ParamsRole>());
                    i++;
                }
            }
            if (node["requirePermission"])
            {
                rhs.requirePermission = node["requirePermission"].as<int>();
            }

            return true;
        }

        /**
         * @brief 将 CustomRule 类型数据编码为 YAML 节点。
         * @param rhs 源 CustomRule 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const CustomRule& rhs)
        {
            Node node;

            node["enable"] = rhs.enable;
            node["supportSystemRole"] = rhs.supportSystemRole;
            node["name"] = rhs.name;
            node["model"] = rhs.model;
            node["apiPath"] = rhs.apiPath;
            node["requirePermission"] = rhs.requirePermission;

            node["apiKeyRole"] = rhs.apiKeyRole;

            Node headersNode;
            for (const auto& header : rhs.headers)
            {
                headersNode[header.first] = header.second;
            }
            node["headers"] = headersNode;

            Node varsNode;
            for (const auto& var : rhs.vars)
            {
                varsNode.push_back(var);
            }
            node["vars"] = varsNode;

            Node rolesNode;
            for (const auto& role : rhs.roles)
            {
                rolesNode[role.first] = role.second;
            }
            node["roles"] = rolesNode;

            Node extraNode;
            for (const auto& extra : rhs.extraMust)
            {
                extraNode.push_back(extra);
            }
            node["extraMust"] = extraNode;

            node["promptRole"] = rhs.promptRole;
            node["supportModels"] = rhs.supportedModels;

            Node paramsNode(NodeType::Sequence);
            for (const auto& param : rhs.params)
            {
                paramsNode.push_back(param);
            }
            node["params"] = paramsNode;

            node["responseRole"] = rhs.responseRole;
            node["author"] = rhs.author;
            node["version"] = rhs.version;
            node["description"] = rhs.description;

            return node;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ClaudeAPICreateInfo 类型。
     */
    template <>
    struct convert<ClaudeAPICreateInfo>
    {
        /**
         * @brief 将 ClaudeAPICreateInfo 类型数据编码为 YAML 节点。
         * @param data 源 ClaudeAPICreateInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const ClaudeAPICreateInfo& data)
        {
            Node node;
            node["enable"] = data.enable;
            node["apiKey"] = data.apiKey;
            node["model"] = data.model;
            node["apiVersion"] = data.apiVersion;
            node["endPoint"] = data._endPoint;
            node["supportModels"] = data.supportedModels;
            node["requirePermission"] = data.requirePermission;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 ClaudeAPICreateInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 ClaudeAPICreateInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ClaudeAPICreateInfo& data)
        {
            data.enable = node["enable"].as<bool>();
            data.apiKey = node["apiKey"].as<std::string>();
            data.model = node["model"].as<std::string>();
            data.apiVersion = node["apiVersion"].as<std::string>();
            data._endPoint = node["endPoint"].as<std::string>();
            if (node["supportModels"] && node["supportModels"].IsSequence())
            {
                data.supportedModels.clear();
                for (const auto& model : node["supportModels"])
                {
                    data.supportedModels.push_back(model.as<std::string>());
                }
            }
            else
            {
                data.supportedModels = {"claude-3.5", "claude-3", "claude-2"};
            }
            if (node["requirePermission"])
            {
                data.requirePermission = node["requirePermission"].as<int>();
            }
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 LLamaCreateInfo 类型。
     */
    template <>
    struct convert<LLamaCreateInfo>
    {
        /**
         * @brief 将 LLamaCreateInfo 类型数据编码为 YAML 节点。
         * @param data 源 LLamaCreateInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const LLamaCreateInfo& data)
        {
            Node node;
            node["model"] = data.model;
            node["contextSize"] = data.contextSize;
            node["maxTokens"] = data.maxTokens;
            node["requirePermission"] = data.requirePermission;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 LLamaCreateInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 LLamaCreateInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, LLamaCreateInfo& data)
        {
            data.model = node["model"].as<std::string>();
            data.contextSize = node["contextSize"].as<int>();
            data.maxTokens = node["maxTokens"].as<int>();
            if (node["requirePermission"])
            {
                data.requirePermission = node["requirePermission"].as<int>();
            }
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 GPTLikeCreateInfo 类型。
     */
    template <>
    struct convert<GPTLikeCreateInfo>
    {
        /**
         * @brief 将 GPTLikeCreateInfo 类型数据编码为 YAML 节点。
         * @param data 源 GPTLikeCreateInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const GPTLikeCreateInfo& data)
        {
            Node node;
            node["enable"] = data.enable;
            node["api_key"] = data.api_key;
            node["model"] = data.model;
            node["apiHost"] = data.apiHost;
            node["apiPath"] = data.apiPath;
            node["useLocalModel"] = data.useLocalModel;
            node["llamaData"] = data.llamaData;
            node["supportModels"] = data.supportedModels;
            node["requirePermission"] = data.requirePermission;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 GPTLikeCreateInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 GPTLikeCreateInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, GPTLikeCreateInfo& data)
        {
            data.enable = node["enable"].as<bool>();
            data.api_key = node["api_key"].as<std::string>();
            data.model = node["model"].as<std::string>();
            if (node["requirePermission"])
            {
                data.requirePermission = node["requirePermission"].as<int>();
            }
            if (node["apiHost"])
                data.apiHost = node["apiHost"].as<std::string>();
            if (node["apiPath"])
                data.apiPath = node["apiPath"].as<std::string>();
            if (node["useLocalModel"])
                data.useLocalModel = node["useLocalModel"].as<bool>();
            if (node["llamaData"])
                data.llamaData = node["llamaData"].as<LLamaCreateInfo>();
            if (node["supportModels"] && node["supportModels"].IsSequence())
            {
                data.supportedModels.clear();
                for (const auto& model : node["supportModels"])
                {
                    data.supportedModels.push_back(model.as<std::string>());
                }
            }
            else
            {
                data.supportedModels = {"grok-1.0", "mistral-7b", "mistral-7b-chat", "mistral-7b-instruct-v1"};
            }
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 GeminiBotCreateInfo 类型。
     */
    template <>
    struct convert<GeminiBotCreateInfo>
    {
        /**
         * @brief 将 GeminiBotCreateInfo 类型数据编码为 YAML 节点。
         * @param data 源 GeminiBotCreateInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const GeminiBotCreateInfo& data)
        {
            Node node;
            node["enable"] = data.enable;
            node["api_Key"] = data._apiKey;
            node["endPoint"] = data._endPoint;
            node["model"] = data.model;
            node["supportModels"] = data.supportedModels;
            node["requirePermission"] = data.requirePermission;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 GeminiBotCreateInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 GeminiBotCreateInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, GeminiBotCreateInfo& data)
        {
            data._apiKey = node["api_Key"].as<std::string>();
            data.enable = node["enable"].as<bool>();
            data._endPoint = node["endPoint"].as<std::string>();
            if (node["requirePermission"])
            {
                data.requirePermission = node["requirePermission"].as<int>();
            }
            if (node["model"])
            {
                data.model = node["model"].as<std::string>();
            }
            else
            {
                data.model = "gemini-2.0-flash";
            }
            if (node["supportModels"] && node["supportModels"].IsSequence())
            {
                data.supportedModels.clear();
                for (const auto& model : node["supportModels"])
                {
                    data.supportedModels.push_back(model.as<std::string>());
                }
            }
            else
            {
                data.supportedModels = {"gemini-2.0-flash", "gemini-1.5"};
            }
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 ClaudeBotCreateInfo 类型。
     */
    template <>
    struct convert<ClaudeBotCreateInfo>
    {
        /**
         * @brief 将 ClaudeBotCreateInfo 类型数据编码为 YAML 节点。
         * @param data 源 ClaudeBotCreateInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const ClaudeBotCreateInfo& data)
        {
            Node node;
            node["enable"] = data.enable;
            node["channelID"] = data.channelID;
            node["userName"] = data.userName;
            node["cookies"] = data.cookies;
            node["slackToken"] = data.slackToken;
            node["supportModels"] = data.supportedModels;
            node["requirePermission"] = data.requirePermission;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 ClaudeBotCreateInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 ClaudeBotCreateInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, ClaudeBotCreateInfo& data)
        {
            if (!node["channelID"])
            {
                return false;
            }
            if (node["requirePermission"])
            {
                data.requirePermission = node["requirePermission"].as<int>();
            }
            data.cookies = node["cookies"].as<std::string>();
            data.userName = node["userName"].as<std::string>();
            data.enable = node["enable"].as<bool>();
            data.channelID = node["channelID"].as<std::string>();
            data.slackToken = node["slackToken"].as<std::string>();
            if (node["supportModels"] && node["supportModels"].IsSequence())
            {
                data.supportedModels.clear();
                for (const auto& model : node["supportModels"])
                {
                    data.supportedModels.push_back(model.as<std::string>());
                }
            }
            else
            {
                data.supportedModels = {"claude-3.5", "claude-3", "claude-2"};
            }
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 OpenAIBotCreateInfo 类型。
     */
    template <>
    struct convert<OpenAIBotCreateInfo>
    {
        /**
         * @brief 将 OpenAIBotCreateInfo 类型数据编码为 YAML 节点。
         * @param data 源 OpenAIBotCreateInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const OpenAIBotCreateInfo& data)
        {
            Node node;
            node["enable"] = data.enable;
            node["api_key"] = data.api_key;
            node["model"] = data.model;
            node["proxy"] = data.proxy;
            node["useWebProxy"] = data.useWebProxy;
            node["endPoint"] = data._endPoint;
            node["supportModels"] = data.supportedModels;
            node["requirePermission"] = data.requirePermission;

            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 OpenAIBotCreateInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 OpenAIBotCreateInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, OpenAIBotCreateInfo& data)
        {
            data.enable = node["enable"].as<bool>();
            data.api_key = node["api_key"].as<std::string>();
            data.useWebProxy = node["useWebProxy"].as<bool>();
            if (node["requirePermission"])
            {
                data.requirePermission = node["requirePermission"].as<int>();
            }
            if (node["model"])
            {
                data.model = node["model"].as<std::string>();
            }
            data.proxy = node["proxy"].as<std::string>();
            data._endPoint = node["endPoint"].as<std::string>();
            if (node["supportModels"] && node["supportModels"].IsSequence())
            {
                data.supportedModels.clear();
                for (const auto& model : node["supportModels"])
                {
                    data.supportedModels.push_back(model.as<std::string>());
                }
            }
            else
            {
                data.supportedModels = {"gpt-4o", "gpt-4", "gpt-3.5-turbo"};
            }
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 Configure 类型。
     */
    template <>
    struct convert<Configure>
    {
        /**
         * @brief 将 Configure 类型数据编码为 YAML 节点。
         * @param config 源 Configure 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const Configure& config)
        {
            Node node;
            node["enableAuth"] = config.enableAuth;
            node["admin"] = config.admin;
            node["openAi"] = config.openAi;
            node["claude"] = config.claude;
            node["gemini"] = config.gemini;
            node["grok"] = config.grok;
            node["mistral"] = config.mistral;
            node["qwen"] = config.qianwen;
            node["chatglm"] = config.chatglm;
            node["hunyuan"] = config.hunyuan;
            node["baichuan"] = config.baichuan;
            node["sparkdesk"] = config.sparkdesk;
            node["huoshan"] = config.huoshan;
            node["claudeAPI"] = config.claudeAPI;
            node["customGPTs"] = config.customGPTs;
            node["customRules"] = config.customRules;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 Configure 类型。
         * @param node YAML 节点。
         * @param config 目标 Configure 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, Configure& config)
        {
            if (node["enableAuth"])
            {
                config.enableAuth = node["enableAuth"].as<bool>();
            }
            if (node["admin"])
            {
                config.admin = node["admin"].as<UserInfo>();
            }
            if (node["claudeAPI"])
            {
                config.claudeAPI = node["claudeAPI"].as<ClaudeAPICreateInfo>();
            }
            if (node["mistral"])
            {
                config.mistral = node["mistral"].as<GPTLikeCreateInfo>();
            }
            if (node["qwen"])
            {
                config.qianwen = node["qwen"].as<GPTLikeCreateInfo>();
            }
            if (node["sparkdesk"])
            {
                config.sparkdesk = node["sparkdesk"].as<GPTLikeCreateInfo>();
            }
            if (node["chatglm"])
            {
                config.chatglm = node["chatglm"].as<GPTLikeCreateInfo>();
            }
            if (node["hunyuan"])
            {
                config.hunyuan = node["hunyuan"].as<GPTLikeCreateInfo>();
            }
            if (node["baichuan"])
            {
                config.baichuan = node["baichuan"].as<GPTLikeCreateInfo>();
            }
            if (node["huoshan"])
            {
                config.huoshan = node["huoshan"].as<GPTLikeCreateInfo>();
            }
            if (node["openAi"])
                config.openAi = node["openAi"].as<OpenAIBotCreateInfo>();
            if (node["gemini"])
                config.gemini = node["gemini"].as<GeminiBotCreateInfo>();
            if (node["claude"])
                config.claude = node["claude"].as<ClaudeBotCreateInfo>();
            if (node["grok"])
                config.grok = node["grok"].as<GPTLikeCreateInfo>();
            if (node["customGPTs"])
                config.customGPTs = node["customGPTs"].as<std::unordered_map<std::string, GPTLikeCreateInfo>>();
            if (node["customRules"])
                config.customRules = node["customRules"].as<std::vector<CustomRule>>();
            return true;
        }
    };

    /**
     * @brief YAML 转换器，用于序列化和反序列化 UserInfo 类型。
     */
    template <>
    struct convert<UserInfo>
    {
        /**
         * @brief 将 UserInfo 类型数据编码为 YAML 节点。
         * @param data 源 UserInfo 类型对象。
         * @return 编码后的 YAML 节点。
         */
        static Node encode(const UserInfo& data)
        {
            Node node;
            node["nickname"] = data.nickName;
            node["password"] = data.passwd;
            node["permission"] = data.permissions;
            Node keys;
            for (const auto& key : data.authKeys)
            {
                keys.push_back(key);
            }
            node["money"] = data.money;
            return node;
        }

        /**
         * @brief 从 YAML 节点解码数据到 UserInfo 类型。
         * @param node YAML 节点。
         * @param data 目标 UserInfo 类型对象。
         * @return 如果解码成功返回 true，否则返回 false。
         */
        static bool decode(const Node& node, UserInfo& data)
        {
            if (node["nickname"])
            {
                data.nickName = node["nickname"].as<std::string>();
            }
            if (node["password"])
            {
                data.passwd = node["password"].as<std::string>();
            }
            if (node["permission"])
            {
                data.permissions = node["permission"].as<int>();
            }
            if (node["money"])
            {
                data.money = node["money"].as<int>();
            }
            if (node["authKeys"])
            {
                data.authKeys.clear();
                for (const auto& key : node["authKeys"])
                {
                    data.authKeys.push_back(key.as<std::string>());
                }
            }
            return true;
        }
    };
}

#endif