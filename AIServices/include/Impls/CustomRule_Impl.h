#ifndef CUSTOMROLE_IMPL_H
#define CUSTOMROLE_IMPL_H
#include "ChatBot.h"

/**
 * @brief 用于构建JSON路径和值的工具类。
 */
class JsonPathBuilder
{
private:
    json rootJson; ///< 根JSON对象。

    /**
     * @brief 在指定路径添加值到JSON对象。
     * @param jsonObj 要修改的JSON对象。
     * @param path 路径的字符串向量。
     * @param value 要添加的值。
     */
    void addValueAtPath(json& jsonObj, const std::vector<std::string>& path, const std::string& value);

public:
    /**
     * @brief 添加一个路径和对应的值到JSON结构中。
     * @param pathStr 要添加的路径字符串。
     * @param value 要添加的值。
     */
    void addPath(const std::string& pathStr, const std::string& value);

    /**
     * @brief 获取构建好的JSON对象。
     * @return 构建好的JSON对象。
     */
    json getJson();
};

/**
 * @brief 自定义规则的聊天机器人实现类。
 * @details 继承自ChatBot，提供基于自定义规则的聊天机器人功能。
 */
class CustomRule_Impl : public ChatBot
{
private:
    CustomRule CustomRuleData; ///< 自定义规则数据。
    const std::string ConversationPath = "Conversations/CustomRule/"; ///< 对话存储路径。
    json SystemPrompt; ///< 系统提示。
    const std::string suffix = ".dat"; ///< 文件后缀。
    std::string convid_ = "default"; ///< 当前对话ID。
    std::map<std::string, json> Conversation; ///< 所有对话的映射。
    json history; ///< 对话历史。
    json templateJson; ///< 模板JSON对象。
    std::vector<std::string> paths; ///< 路径列表1。
    std::vector<std::string> paths2; ///< 路径列表2。

    /**
     * @brief 发送请求到后端服务。
     * @param data 要发送的数据。
     * @param ts 时间戳。
     * @return 响应字符串。
     */
    std::string sendRequest(std::string data, size_t ts) override;

    /**
     * @brief 构建请求的JSON对象。
     * @param prompt 用户提示。
     * @param role 角色。
     * @return 构建好的请求JSON对象。
     */
    json buildRequest(const std::string& prompt, const std::string& role);

public:
    /**
     * @brief 构造函数，初始化自定义规则实现。
     * @param data 自定义规则数据。
     * @param systemrole 系统角色提示，默认为"You are a ai assistant made by Artiverse Studio."。
     */
    CustomRule_Impl(const CustomRule& data,
                    std::string systemrole = "You are a ai assistant made by Artiverse Studio.");
    /**
     * @brief 提交一个请求并获取响应。
     * @param prompt 用户提示。
     * @param timeStamp 时间戳。
     * @param role 角色。
     * @param convid 对话ID。
     * @param temp 温度参数。
     * @param top_p top_p参数。
     * @param top_k top_k参数。
     * @param pres_pen 存在惩罚参数。
     * @param freq_pen 频率惩罚参数。
     * @param async 是否异步。
     * @return 响应字符串。
     */
    std::string Submit(std::string prompt, size_t timeStamp, std::string role,
                       std::string convid, float temp,
                       float top_p,
                       uint32_t top_k,
                       float pres_pen,
                       float freq_pen, bool async) override;
    /**
     * @brief 重置当前对话状态。
     */
    void Reset() override;
    /**
     * @brief 加载指定名称的对话。
     * @param name 对话名称。
     */
    void Load(std::string name) override;
    /**
     * @brief 保存当前对话到指定名称。
     * @param name 对话名称。
     */
    void Save(std::string name) override;
    /**
     * @brief 删除指定名称的对话。
     * @param name 对话名称。
     */
    void Del(std::string name) override;
    /**
     * @brief 添加一个新对话。
     * @param name 新对话的名称。
     */
    void Add(std::string name) override;
    /**
     * @brief 获取对话历史。
     * @return 包含时间戳和消息的对话历史映射。
     */
    std::map<long long, std::string> GetHistory() override;
    /**
     * @brief 构建对话历史。
     * @param history 包含角色和消息对的对话历史。
     */
    void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) override;

    /**
     * @brief 获取当前使用的模型名称。
     * @return 模型名称字符串。
     */
    std::string GetModel() override
    {
        return CustomRuleData.model;
    }

    /**
     * @brief 获取所有对话的名称。
     * @return 包含所有对话名称的字符串向量。
     */
    std::vector<std::string> GetAllConversations() override;
};

#endif