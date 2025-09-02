#ifndef CLAUDE_IMPL_H
#define CLAUDE_IMPL_H

#include "ChatBot.h"

/**
 * @brief ClaudeInSlack 类，用于在 Slack 环境中与 Claude 聊天机器人交互。
 *
 * 继承自 ChatBot 接口，提供在 Slack 平台提交请求、管理会话等功能。
 */
class ClaudeInSlack : public ChatBot
{
public:
    /**
     * @brief 构造函数，使用 ClaudeBotCreateInfo 初始化 ClaudeInSlack 实例。
     * @param data 包含 Claude 机器人创建信息的结构体。
     */
    ClaudeInSlack(const ClaudeBotCreateInfo& data) : claudeData(data)
    {
    }

    /**
     * @brief 提交一个提示（prompt）给 Claude 机器人并获取响应。
     * @param prompt 用户输入的提示或消息。
     * @param timeStamp 消息的时间戳。
     * @param role 消息发送者的角色（例如 "User"）。
     * @param convid 会话ID，用于区分不同的对话。
     * @param temp 温度参数，控制生成文本的随机性。
     * @param top_p Top-P 采样参数，控制生成文本的多样性。
     * @param top_k Top-K 采样参数，控制生成文本的多样性。
     * @param pres_pen 存在惩罚，用于减少重复。
     * @param freq_pen 频率惩罚，用于减少重复。
     * @param async 是否异步提交请求。
     * @return 机器人生成的响应字符串。
     */
    std::string Submit(std::string prompt, size_t timeStamp, std::string role = Role::User,
                       std::string convid = "default", float temp = 0.7f,
                       float top_p = 0.9f,
                       uint32_t top_k = 40u,
                       float pres_pen = 0.0f,
                       float freq_pen = 0.0f, bool async = false) override;

    /**
     * @brief 重置当前会话或机器人状态。
     */
    void Reset() override;;

    /**
     * @brief 加载指定名称的会话或配置。
     * @param name 要加载的会话或配置的名称。
     */
    void Load(std::string name) override;

    /**
     * @brief 保存当前会话或配置到指定名称。
     * @param name 要保存的会话或配置的名称。
     */
    void Save(std::string name) override;

    /**
     * @brief 删除指定ID的会话。
     * @param id 要删除的会话ID。
     */
    void Del(std::string id) override;

    /**
     * @brief 添加一个新会话或配置。
     * @param name 要添加的会话或配置的名称。
     */
    void Add(std::string name) override;

    /**
     * @brief 获取当前会话的历史记录。
     * @return 包含时间戳和消息内容的映射。
     */
    map<long long, string> GetHistory() override;

    /**
     * @brief 发送原始请求数据并获取响应。
     * @param data 要发送的请求数据。
     * @param ts 请求的时间戳。
     * @return 响应字符串。
     */
    std::string sendRequest(std::string data, size_t ts) override
    {
        return "";
    }

    /**
     * @brief 获取当前使用的模型名称。
     * @return 模型名称字符串，例如 "Claude"。
     */
    std::string GetModel() override
    {
        return "Claude";
    }

    /**
     * @brief 根据提供的历史记录构建会话历史。
     * @param history 包含消息对（角色-内容）的向量。
     */
    void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) override
    {
    }

    /**
     * @brief 获取所有可用的会话列表。
     * @return 包含所有会话名称或ID的字符串向量。
     */
    std::vector<std::string> GetAllConversations() override;

private:
    map<string, string> ChannelListName; ///< 频道名称到ID的映射。
    map<string, string> ChannelListID; ///< 频道ID到名称的映射。
    ClaudeBotCreateInfo claudeData; ///< Claude 机器人创建信息。
};

/**
 * @brief Claude 类，用于与 Claude API 交互的聊天机器人实现。
 *
 * 继承自 ChatBot 接口，提供通过 API 提交请求、管理会话等功能。
 */
class Claude : public ChatBot
{
public:
    /**
     * @brief 构造函数，使用指定的系统角色初始化 Claude 实例。
     * @param systemrole 机器人的系统角色描述。
     */
    Claude(std::string systemrole);

    /**
     * @brief 构造函数，使用 Claude API 创建信息和可选的系统角色初始化 Claude 实例。
     * @param claude_data 包含 Claude API 创建信息的结构体。
     * @param systemrole 机器人的系统角色描述。
     */
    Claude(const ClaudeAPICreateInfo& claude_data, std::string systemrole = "");

    /**
     * @brief 提交一个提示（prompt）给 Claude 机器人并获取响应。
     * @param prompt 用户输入的提示或消息。
     * @param timeStamp 消息的时间戳。
     * @param role 消息发送者的角色（例如 "User"）。
     * @param convid 会话ID，用于区分不同的对话。
     * @param temp 温度参数，控制生成文本的随机性。
     * @param top_p Top-P 采样参数，控制生成文本的多样性。
     * @param top_k Top-K 采样参数，控制生成文本的多样性。
     * @param pres_pen 存在惩罚，用于减少重复。
     * @param freq_pen 频率惩罚，用于减少重复。
     * @param async 是否异步提交请求。
     * @return 机器人生成的响应字符串。
     */
    std::string Submit(std::string prompt, size_t timeStamp, std::string role = Role::User,
                       std::string convid = "default", float temp = 0.7f,
                       float top_p = 0.9f,
                       uint32_t top_k = 40u,
                       float pres_pen = 0.0f,
                       float freq_pen = 0.0f, bool async = false) override;

    /**
     * @brief 重置当前会话或机器人状态。
     */
    void Reset() override;

    /**
     * @brief 加载指定名称的会话或配置。
     * @param name 要加载的会话或配置的名称。
     */
    void Load(std::string name) override;

    /**
     * @brief 保存当前会话或配置到指定名称。
     * @param name 要保存的会话或配置的名称。
     */
    void Save(std::string name) override;

    /**
     * @brief 删除指定名称的会话。
     * @param name 要删除的会话名称。
     */
    void Del(std::string name) override;

    /**
     * @brief 添加一个新会话或配置。
     * @param name 要添加的会话或配置的名称。
     */
    void Add(std::string name) override;

    /**
     * @brief 获取当前会话的历史记录。
     * @return 包含时间戳和消息内容的映射。
     */
    map<long long, string> GetHistory() override;

    /**
     * @brief 将时间戳转换为可读的时间字符串。
     * @param timestamp 要转换的时间戳。
     * @return 格式化的时间字符串。
     */
    static std::string Stamp2Time(long long timestamp);

    /**
     * @brief 发送原始请求数据并获取响应。
     * @param data 要发送的请求数据。
     * @param ts 请求的时间戳。
     * @return 响应字符串。
     */
    std::string sendRequest(std::string data, size_t ts) override;

    json history; ///< 当前会话的历史记录。

protected:
    ClaudeAPICreateInfo claude_data_; ///< Claude API 创建信息。
    std::string mode_name_ = "default"; ///< 当前模式名称。
    std::string convid_ = "default"; ///< 当前会话ID。
    std::map<std::string, json> Conversation; ///< 所有会话的映射。
    std::mutex print_mutex; ///< 用于打印操作的互斥锁。
    const std::string ConversationPath = "Conversations/Claude/"; ///< 会话存储路径。
    const std::string sys = "You are Claude, an AI assistant developed by Anthropic. Please respond in Chinese.";
    ///< 默认系统角色提示。
    const std::string suffix = ".dat"; ///< 会话文件后缀。
    json LastHistory; ///< 上一次的历史记录。
    json defaultJson; ///< 默认的 JSON 对象。

    /**
     * @brief 检查当前会话是否已保存。
     * @return 如果已保存则返回 true，否则返回 false。
     */
    bool IsSaved();

    /**
     * @brief 获取当前的 Unix 时间戳。
     * @return 当前的 Unix 时间戳（毫秒）。
     */
    static long long getCurrentTimestamp();

    /**
     * @brief 获取指定天数之前的 Unix 时间戳。
     * @param daysBefore 距离当前时间的天数。
     * @return 指定天数之前的 Unix 时间戳（毫秒）。
     */
    static long long getTimestampBefore(int daysBefore);

public:
    /**
     * @brief 根据提供的历史记录构建会话历史。
     * @param history 包含消息对（角色-内容）的向量。
     */
    void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) override;

    /**
     * @brief 获取当前使用的模型名称。
     * @return 模型名称字符串。
     */
    std::string GetModel() override
    {
        return claude_data_.model;
    }

    /**
     * @brief 获取所有可用的会话列表。
     * @return 包含所有会话名称或ID的字符串向量。
     */
    std::vector<std::string> GetAllConversations() override;
};

#endif
