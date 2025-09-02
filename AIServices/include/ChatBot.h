#ifndef CHATBOT_H
#define CHATBOT_H

#include "Utils.h"
#include <llama.h>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Configure.h"
#include "Logger.h"
#include <curl/curl.h>
#include <fstream>
using json = nlohmann::json;
using namespace std;

/**
 * @brief 定义了聊天中不同角色的命名空间。
 */
namespace Role
{
    static std::string User = "user"; ///< 用户角色。
    static std::string System = "system"; ///< 系统角色。
    static std::string Assistant = "assistant"; ///< 助手角色。
};

/**
 * @brief 存储计费信息的结构体。
 */
struct Billing
{
    float total = -1; ///< 总金额或总额度。
    float available = -1; ///< 可用金额或可用额度。
    float used = -1; ///< 已使用金额或已使用额度。
    long long date = -1; ///< 计费日期或时间戳。
};

/**
 * @brief 用于存储和管理字符串指针及相关实例和互斥锁的结构体。
 */
struct DString
{
    std::string* str1; ///< 第一个字符串指针。
    std::string* str2; ///< 第二个字符串指针。
    std::string* response; ///< 响应字符串指针。

    void* instance; ///< 相关实例的通用指针。
    std::mutex mtx; ///< 用于保护结构体成员访问的互斥锁。
};

/**
 * @brief 聊天机器人基类，定义了聊天机器人所需的核心功能接口。
 */
class ChatBot
{
public:
    friend class StringExecutor;
    /**
     * @brief 提交用户提示并获取响应。
     * @param prompt 用户输入的提示信息。
     * @param timeStamp 消息的时间戳。
     * @param role 消息发送者的角色（例如：用户、系统、助手）。
     * @param convid 会话ID，用于区分不同的对话。
     * @param temp 生成文本的温度参数，影响随机性。
     * @param top_p Top-P采样参数，影响生成文本的多样性。
     * @param top_k Top-K采样参数，影响生成文本的多样性。
     * @param pres_pen 存在惩罚参数，用于减少重复。
     * @param freq_pen 频率惩罚参数，用于减少重复。
     * @param async 是否异步提交请求。
     * @return 聊天机器人的响应字符串。
     */
    virtual std::string Submit(std::string prompt, size_t timeStamp, std::string role = Role::User,
                               std::string convid = "default", float temp = 0.7f,
                               float top_p = 0.9f,
                               uint32_t top_k = 40u,
                               float pres_pen = 0.0f,
                               float freq_pen = 0.0f, bool async = false) = 0;

    /**
     * @brief 根据提供的历史记录构建聊天上下文。
     * @param history 包含对话历史的字符串对向量。
     */
    virtual void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) =0;
    /**
     * @brief 获取当前聊天机器人使用的模型名称。
     * @return 模型名称字符串。
     */
    virtual std::string GetModel() =0;

    /**
     * @brief 异步提交用户提示并获取响应。
     * @param prompt 用户输入的提示信息。
     * @param timeStamp 消息的时间戳。
     * @param role 消息发送者的角色（例如：用户、系统、助手）。
     * @param convid 会话ID，用于区分不同的对话。
     * @param temp 生成文本的温度参数，影响随机性。
     * @param top_p Top-P采样参数，影响生成文本的多样性。
     * @param top_k Top-K采样参数，影响生成文本的多样性。
     * @param pres_pen 存在惩罚参数，用于减少重复。
     * @param freq_pen 频率惩罚参数，用于减少重复。
     */
    void SubmitAsync(
        std::string prompt,
        size_t timeStamp,
        std::string role = Role::User,
        std::string convid = "default",
        float temp = 0.7f,
        float top_p = 0.9f,
        uint32_t top_k = 40u,
        float pres_pen = 0.0f,
        float freq_pen = 0.0f)
    {
        {
            std::lock_guard<std::mutex> lock(forceStopMutex);
            forceStop = false;
        }
        lastFinalResponse = "";
        std::get<1>(Response[timeStamp]) = false;
        std::thread([=,this]
        {
            Submit(prompt, timeStamp, role, convid, temp, top_p, top_k, pres_pen, freq_pen, true);
        }).detach();
    }

    /**
     * @brief 获取上次的原始响应字符串。
     * @return 上次的原始响应字符串。
     */
    std::string GetLastRawResponse()
    {
        std::string response = lastRawResponse;
        lastRawResponse = "";
        return response;
    }

    /**
     * @brief 强制停止当前正在进行的生成或请求。
     */
    void ForceStop()
    {
        std::lock_guard<std::mutex> lock(forceStopMutex);
        forceStop = true;
    }

    /**
     * @brief 重置聊天机器人的状态或会话。
     */
    virtual void Reset() = 0;

    /**
     * @brief 加载指定名称的会话或配置。
     * @param name 要加载的会话名称。
     */
    virtual void Load(std::string name = "default") = 0;

    /**
     * @brief 保存指定名称的会话或配置。
     * @param name 要保存的会话名称。
     */
    virtual void Save(std::string name = "default") = 0;

    /**
     * @brief 删除指定名称的会话。
     * @param name 要删除的会话名称。
     */
    virtual void Del(std::string name) = 0;

    /**
     * @brief 添加一个新会话。
     * @param name 要添加的会话名称。
     */
    virtual void Add(std::string name) = 0;

    /**
     * @brief 获取所有已保存的会话名称列表。
     * @return 包含所有会话名称的字符串向量。
     */
    virtual std::vector<std::string> GetAllConversations() = 0;

    /**
     * @brief 根据唯一ID获取聊天机器人的响应。
     * @param uid 响应的唯一标识符。
     * @return 对应的响应字符串。
     */
    std::string GetResponse(size_t uid)
    {
        std::string response;
        response = std::get<0>(Response[uid]);
        lastFinalResponse += response;
        std::get<0>(Response[uid]) = "";
        return response;
    }

    /**
     * @brief 检查指定ID的响应是否已完成。
     * @param uid 响应的唯一标识符。
     * @return 如果响应已完成则返回true，否则返回false。
     */
    bool Finished(size_t uid)
    {
        return std::get<1>(Response[uid]);
    }

    /**
     * @brief 获取最后一次完整的响应字符串。
     * @return 最后一次完整的响应字符串。
     */
    std::string GetLastFinalResponse()
    {
        return lastFinalResponse;
    }

    /**
     * @brief 获取聊天历史记录。
     * @return 包含时间戳和消息内容的映射。
     */
    virtual map<long long, string> GetHistory() = 0;

    map<long long, string> History; ///< 存储聊天历史记录的映射。
    std::atomic<bool> forceStop{false}; ///< 原子布尔变量，用于指示是否强制停止操作。
    /**
     * @brief 发送请求到后端服务。
     * @param data 要发送的请求数据。
     * @param ts 请求的时间戳。
     * @return 后端服务的响应字符串。
     */
    virtual std::string sendRequest(std::string data, size_t ts) =0;

protected:
    long long lastTimeStamp = 0; ///< 上次操作的时间戳。
    std::mutex fileAccessMutex; ///< 用于文件访问的互斥锁。
    std::mutex historyAccessMutex; ///< 用于历史记录访问的互斥锁。
    std::string lastFinalResponse; ///< 最后一次完整的响应。
    std::string lastRawResponse; ///< 最后一次原始的响应。
    unordered_map<size_t, std::tuple<std::string, bool>> Response; ///< 存储响应的映射，键为UID，值为响应字符串和完成状态。

    std::mutex forceStopMutex; ///< 用于保护forceStop变量的互斥锁。
};

/**
 * @brief cURL库的进度回调函数，用于检查是否需要强制停止传输。
 * @param clientp 用户自定义数据指针，通常指向ChatBot实例。
 * @param dltotal 预期下载的总字节数。
 * @param dlnow 当前已下载的字节数。
 * @param ultotal 预期上传的总字节数。
 * @param ulnow 当前已上传的字节数。
 * @return 如果需要停止传输则返回1，否则返回0。
 */
inline static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                                   curl_off_t ulnow)
{
    ChatBot* chatBot = static_cast<ChatBot*>(clientp);
    if (chatBot && chatBot->forceStop.load())
    {
        return 1;
    }
    return 0;
}

#endif