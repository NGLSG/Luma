#ifndef CHATGPT_H
#define CHATGPT_H

#include "ChatBot.h"

/**
 * @brief ChatGPT 类，继承自 ChatBot，用于与 OpenAI 的 ChatGPT 模型进行交互。
 *
 * 提供了提交请求、管理会话历史、加载/保存配置等功能。
 */
class ChatGPT : public ChatBot
{
public:
    /**
     * @brief 构造函数，使用指定的系统角色初始化 ChatGPT 实例。
     * @param systemrole 系统角色字符串，定义了助手的行为。
     */
    ChatGPT(std::string systemrole);

    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 ChatGPT 实例。
     * @param chat_data 包含 OpenAI 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串，定义了助手的行为。
     */
    ChatGPT(const OpenAIBotCreateInfo& chat_data, std::string systemrole = "");

    /**
     * @brief 提交用户提示给 ChatGPT 模型并获取响应。
     * @param prompt 用户输入的提示信息。
     * @param timeStamp 时间戳。
     * @param role 消息的角色（例如 "user", "system"）。
     * @param convid 会话ID，用于区分不同的对话。
     * @param temp 温度参数，控制生成文本的随机性。
     * @param top_p top_p 参数，控制生成文本的多样性。
     * @param top_k top_k 参数，控制生成文本的多样性。
     * @param pres_pen 惩罚重复词语的参数。
     * @param freq_pen 惩罚频繁词语的参数。
     * @param async 是否异步提交请求。
     * @return 模型生成的响应字符串。
     */
    std::string Submit(std::string prompt, size_t timeStamp, std::string role,
                       std::string convid, float temp,
                       float top_p,
                       uint32_t top_k,
                       float pres_pen,
                       float freq_pen, bool async) override;

    /**
     * @brief 重置当前会话状态。
     */
    void Reset() override;

    /**
     * @brief 根据名称加载会话或配置。
     * @param name 要加载的会话或配置的名称。
     */
    void Load(std::string name) override;

    /**
     * @brief 根据名称保存当前会话或配置。
     * @param name 要保存的会话或配置的名称。
     */
    void Save(std::string name) override;

    /**
     * @brief 根据名称删除会话或配置。
     * @param name 要删除的会话或配置的名称。
     */
    void Del(std::string name) override;

    /**
     * @brief 根据名称添加会话或配置。
     * @param name 要添加的会话或配置的名称。
     */
    void Add(std::string name) override;

    /**
     * @brief 获取当前会话的历史记录。
     * @return 包含时间戳和消息内容的会话历史映射。
     */
    map<long long, string> GetHistory() override;

    /**
     * @brief 将时间戳转换为可读的时间字符串。
     * @param timestamp 要转换的时间戳。
     * @return 格式化的时间字符串。
     */
    static std::string Stamp2Time(long long timestamp);

    json history; ///< 会话历史记录的 JSON 对象。
    /**
     * @brief 发送请求数据到模型。
     * @param data 要发送的请求数据字符串。
     * @param ts 时间戳。
     * @return 模型返回的响应字符串。
     */
    std::string sendRequest(std::string data, size_t ts) override;
    /**
     * @brief 根据提供的历史记录构建内部会话历史。
     * @param history 包含角色和消息对的会话历史向量。
     */
    void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) override;

    /**
     * @brief 获取当前使用的模型名称。
     * @return 模型名称字符串。
     */
    std::string GetModel() override
    {
        return chat_data_.model;
    }

    /**
     * @brief 获取所有已保存的会话列表。
     * @return 包含所有会话名称的字符串向量。
     */
    std::vector<std::string> GetAllConversations() override;

protected:
    OpenAIBotCreateInfo chat_data_; ///< OpenAI 机器人创建信息。
    std::string mode_name_ = "default"; ///< 当前模式名称。
    std::string convid_ = "default"; ///< 当前会话ID。
    std::map<std::string, json> Conversation; ///< 存储所有会话的映射。
    std::mutex print_mutex; ///< 用于保护打印操作的互斥锁。
    const std::string ConversationPath = "Conversations/"; ///< 会话文件存储路径。
    const std::string sys = "You are ChatGPT, a large language model trained by OpenAI. Respond conversationally."; ///< 默认系统角色提示。
    const std::string suffix = ".dat"; ///< 会话文件后缀。

    json defaultJson; ///< 默认的 JSON 对象。
};

/**
 * @brief GPTLike 类，继承自 ChatGPT，用于与类 GPT 模型进行交互。
 *
 * 封装了特定于类 GPT 模型的端点和配置。
 */
class GPTLike : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 GPTLike 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    GPTLike(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;
        chat_data_._endPoint = data.apiHost + data.apiPath;
    }
};

/**
 * @brief Grok 类，继承自 ChatGPT，用于与 Grok 模型进行交互。
 *
 * 封装了特定于 Grok 模型的端点和配置。
 */
class Grok : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 Grok 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    Grok(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;
        chat_data_._endPoint = "https://api.x.ai/v1/chat/completions";
    }
};

/**
 * @brief Mistral 类，继承自 ChatGPT，用于与 Mistral 模型进行交互。
 *
 * 封装了特定于 Mistral 模型的端点和配置。
 */
class Mistral : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 Mistral 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    Mistral(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://api.mistral.ai/v1/chat/completions";
    }
};

/**
 * @brief TongyiQianwen 类，继承自 ChatGPT，用于与通义千问模型进行交互。
 *
 * 封装了特定于通义千问模型的端点和配置。
 */
class TongyiQianwen : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 TongyiQianwen 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    TongyiQianwen(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";


        if (!data.apiHost.empty())
        {
            chat_data_._endPoint = data.apiHost;
        }
    }
};

/**
 * @brief SparkDesk 类，继承自 ChatGPT，用于与星火大模型进行交互。
 *
 * 封装了特定于星火大模型的端点和配置。
 */
class SparkDesk : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 SparkDesk 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    SparkDesk(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://spark-api-open.xf-yun.com/v1/chat/completions";


        if (!data.apiHost.empty())
        {
            chat_data_._endPoint = data.apiHost;
        }
    }
};

/**
 * @brief BaichuanAI 类，继承自 ChatGPT，用于与百川大模型进行交互。
 *
 * 封装了特定于百川大模型的端点和配置。
 */
class BaichuanAI : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 BaichuanAI 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    BaichuanAI(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://api.baichuan-ai.com/v1/chat/completions";


        if (!data.apiHost.empty())
        {
            chat_data_._endPoint = data.apiHost;
        }
    }
};

/**
 * @brief HunyuanAI 类，继承自 ChatGPT，用于与混元大模型进行交互。
 *
 * 封装了特定于混元大模型的端点和配置。
 */
class HunyuanAI : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 HunyuanAI 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    HunyuanAI(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://api.hunyuan.cloud.tencent.com/v1/chat/completions";


        if (!data.apiHost.empty())
        {
            chat_data_._endPoint = data.apiHost;
        }
    }
};

/**
 * @brief HuoshanAI 类，继承自 ChatGPT，用于与火山大模型进行交互。
 *
 * 封装了特定于火山大模型的端点和配置。
 */
class HuoshanAI : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 HuoshanAI 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    HuoshanAI(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";


        if (!data.apiHost.empty())
        {
            chat_data_._endPoint = data.apiHost;
        }
    }
};

/**
 * @brief ChatGLM 类，继承自 ChatGPT，用于与智谱清言（ChatGLM）模型进行交互。
 *
 * 封装了特定于智谱清言模型的端点和配置。
 */
class ChatGLM : public ChatGPT
{
public:
    /**
     * @brief 构造函数，使用创建信息和可选的系统角色初始化 ChatGLM 实例。
     * @param data 包含 GPTLike 机器人创建信息的结构体。
     * @param systemrole 可选的系统角色字符串。
     */
    ChatGLM(const GPTLikeCreateInfo& data, std::string systemrole = "") : ChatGPT(systemrole)
    {
        chat_data_.enable = data.enable;
        chat_data_.api_key = data.api_key;
        chat_data_.model = data.model;
        chat_data_.useWebProxy = true;


        chat_data_._endPoint = "https://open.bigmodel.cn/api/paas/v4/chat/completions";


        if (!data.apiHost.empty())
        {
            chat_data_._endPoint = data.apiHost;
        }
    }
};


#endif