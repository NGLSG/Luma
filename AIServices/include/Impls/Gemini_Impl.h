#ifndef GEMINI_IMPL_H
#define GEMINI_IMPL_H

#include "ChatBot.h"

/**
 * @brief Gemini 类，继承自 ChatBot，用于实现 Gemini 聊天机器人的功能。
 *
 * 该类提供了与 Gemini 模型交互、管理对话历史、加载/保存对话等操作。
 */
class Gemini : public ChatBot
{
public:
    /**
     * @brief 构造函数，初始化 Gemini 聊天机器人实例。
     * @param data 包含 Gemini 机器人创建信息的结构体。
     * @param sys 系统的初始提示或指令。
     */
    Gemini(const GeminiBotCreateInfo& data, const std::string sys);

    /**
     * @brief 发送请求到 Gemini 模型并获取响应。
     * @param data 要发送的请求数据。
     * @param ts 时间戳。
     * @return 模型的响应字符串。
     */
    std::string sendRequest(std::string data, size_t ts) override;

    /**
     * @brief 提交用户提示给 Gemini 模型，并获取回复。
     * @param prompt 用户输入的提示信息。
     * @param timeStamp 请求的时间戳。
     * @param role 消息发送者的角色（例如 "user", "model"）。
     * @param convid 对话ID，用于区分不同的对话。
     * @param temp 温度参数，控制生成文本的随机性。
     * @param top_p Top-P 采样参数，控制生成文本的多样性。
     * @param top_k Top-K 采样参数，控制生成文本的多样性。
     * @param pres_pen 惩罚重复词语的参数。
     * @param freq_pen 惩罚高频词语的参数。
     * @param async 指示是否异步提交请求。
     * @return 模型的回复字符串。
     */
    std::string Submit(std::string prompt, size_t timeStamp, std::string role,
                       std::string convid, float temp,
                       float top_p,
                       uint32_t top_k,
                       float pres_pen,
                       float freq_pen, bool async) override;

    /**
     * @brief 重置当前聊天机器人的状态或对话。
     */
    void Reset() override;

    /**
     * @brief 根据名称加载一个对话。
     * @param name 要加载的对话的名称。
     */
    void Load(std::string name) override;

    /**
     * @brief 根据名称保存当前对话。
     * @param name 要保存的对话的名称。
     */
    void Save(std::string name) override;

    /**
     * @brief 根据名称删除一个对话。
     * @param name 要删除的对话的名称。
     */
    void Del(std::string name) override;

    /**
     * @brief 添加一个新对话或将当前对话添加到列表中。
     * @param name 要添加的对话的名称。
     */
    void Add(std::string name) override;

    /**
     * @brief 获取当前对话的历史记录。
     * @return 包含时间戳和消息内容的对话历史映射。
     */
    map<long long, string> GetHistory() override;

    /**
     * @brief 根据提供的历史记录构建或更新当前对话历史。
     * @param history 包含消息发送者和消息内容的对话历史向量。
     */
    void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) override;

    /**
     * @brief 获取当前使用的模型名称。
     * @return 模型名称字符串。
     */
    std::string GetModel() override;

    /**
     * @brief 获取所有已保存的对话名称列表。
     * @return 包含所有对话名称的字符串向量。
     */
    std::vector<std::string> GetAllConversations() override;

private:
    GeminiBotCreateInfo geminiData; ///< Gemini 机器人创建信息。
    const std::string ConversationPath = "Conversations/Gemini/"; ///< 对话文件存储路径。
    json SystemPrompt; ///< 系统提示，用于初始化模型行为。
    const std::string suffix = ".dat"; ///< 对话文件后缀名。
    std::string convid_ = "default"; ///< 当前对话ID。
    std::map<std::string, json> Conversation; ///< 存储所有对话的映射。
    json history; ///< 当前对话的历史记录。
};

#endif