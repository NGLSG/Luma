#ifndef LLAMA_IMPL_H
#define LLAMA_IMPL_H


#include "ChatBot.h"

/**
 * @brief LLama 类，继承自 ChatBot，用于实现 LLama 语言模型的聊天机器人功能。
 */
class LLama : public ChatBot
{
public:
    /**
     * @brief 构造函数，初始化 LLama 聊天机器人。
     * @param data LLama 创建信息，包含模型路径等配置。
     * @param sysr 系统角色提示，默认为 LLama 模型的介绍。
     */
    LLama(const LLamaCreateInfo& data,
          const std::string& sysr =
              "You are LLama, a large language model trained by OpenSource. Respond conversationally.");

    /**
     * @brief 析构函数，清理 LLama 聊天机器人资源。
     */
    ~LLama();

    /**
     * @brief 提交用户提示并获取 LLama 模型的响应。
     * @param prompt 用户输入的提示信息。
     * @param timeStamp 时间戳。
     * @param role 角色信息。
     * @param convid 会话ID。
     * @param temp 温度参数，控制生成文本的随机性。
     * @param top_p Top-P 采样参数，控制生成文本的多样性。
     * @param top_k Top-K 采样参数，控制生成文本的多样性。
     * @param pres_pen 存在惩罚参数。
     * @param freq_pen 频率惩罚参数。
     * @param async 是否异步提交。
     * @return LLama 模型生成的响应字符串。
     */
    std::string Submit(std::string prompt, size_t timeStamp, std::string role,
                       std::string convid, float temp,
                       float top_p,
                       uint32_t top_k,
                       float pres_pen,
                       float freq_pen, bool async) override;
    /**
     * @brief 重置聊天机器人的状态或会话。
     */
    void Reset() override;
    /**
     * @brief 加载指定名称的会话。
     * @param name 要加载的会话名称。
     */
    void Load(std::string name) override;
    /**
     * @brief 保存当前会话到指定名称。
     * @param name 要保存的会话名称。
     */
    void Save(std::string name) override;
    /**
     * @brief 删除指定名称的会话。
     * @param name 要删除的会话名称。
     */
    void Del(std::string name) override;
    /**
     * @brief 添加指定名称的会话。
     * @param name 要添加的会话名称。
     */
    void Add(std::string name) override;
    /**
     * @brief 获取所有会话的历史记录。
     * @return 包含会话ID和历史字符串的映射。
     */
    map<long long, string> GetHistory() override;
    /**
     * @brief 发送请求数据到模型。
     * @param data 要发送的数据。
     * @param ts 时间戳。
     * @return 模型响应的字符串。
     */
    std::string sendRequest(std::string data, size_t ts) override;

private:
    /**
     * @brief 表示一条聊天消息的结构体。
     */
    struct ChatMessage
    {
        std::string role; ///< 消息的角色（例如 "user", "assistant"）。
        std::string content; ///< 消息的内容。

        /**
         * @brief 将当前 ChatMessage 转换为 llama_chat_message 类型。
         * @return 转换后的 llama_chat_message 对象。
         */
        llama_chat_message To();
    };


    /**
     * @brief 存储聊天信息的结构体。
     */
    struct chatInfo
    {
        std::vector<ChatMessage> messages; ///< 聊天消息列表。
        int prev_len = 0; ///< 上一次消息的长度。

        /**
         * @brief 将当前 chatInfo 转换为 llama_chat_message 向量。
         * @return 转换后的 llama_chat_message 向量。
         */
        std::vector<llama_chat_message> To();
    };

    LLamaCreateInfo llamaData; ///< LLama 模型的创建信息。
    llama_context* ctx; ///< LLama 模型的上下文指针。
    llama_model* model; ///< LLama 模型指针。
    llama_sampler* smpl; ///< LLama 采样器指针。
    std::string convid_ = "default"; ///< 当前会话ID，默认为 "default"。
    const llama_vocab* vocab; ///< LLama 模型的词汇表指针。
    std::vector<char> formatted; ///< 格式化后的数据缓冲区。
    std::string sys = "You are LLama, a large language model trained by OpenSource. Respond conversationally."; ///< 系统提示信息。
    const std::string ConversationPath = "Conversations/"; ///< 会话文件存储路径。
    const std::string suffix = ".dat"; ///< 会话文件后缀名。
    std::unordered_map<std::string, chatInfo> history; ///< 存储所有会话历史的映射。

    /**
     * @brief 获取 GPU 内存使用量。
     * @return GPU 内存使用量（单位可能为 MB 或其他）。
     */
    static uint16_t GetGPUMemory();

    /**
     * @brief 获取 GPU 层数。
     * @return GPU 层数。
     */
    static uint16_t GetGPULayer();

public:
    /**
     * @brief 根据提供的历史记录构建会话。
     * @param history 包含角色和内容的字符串对向量，用于构建历史记录。
     */
    void BuildHistory(const std::vector<std::pair<std::string, std::string>>& history) override;

    /**
     * @brief 获取当前使用的模型名称。
     * @return 模型名称字符串。
     */
    std::string GetModel() override
    {
        return llamaData.model;
    }

    /**
     * @brief 获取所有会话的名称列表。
     * @return 包含所有会话名称的字符串向量。
     */
    std::vector<std::string> GetAllConversations() override;
};


#endif