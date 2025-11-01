#include "Impls/LLama_Impl.h"

#include <vulkan/vulkan.hpp>
#include <llama-context.h>
#include <llama-sampling.h>
#include <iostream>

llama_chat_message LLama::ChatMessage::To()
{
#ifdef _WIN32

    return {role.c_str(), _strdup(content.c_str())};
#else

    return {role.c_str(), strdup(content.c_str())};
#endif
}

std::vector<llama_chat_message> LLama::chatInfo::To()
{
    std::vector<llama_chat_message> msgs;
    for (auto& m : messages)
    {
        msgs.push_back(m.To());
    }
    return msgs;
}

uint16_t LLama::GetGPUMemory()
{
    vk::ApplicationInfo appInfo(
        "GPU Memory Query", VK_MAKE_VERSION(1, 0, 0),
        "No Engine", VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0
    );

    vk::InstanceCreateInfo instanceCreateInfo({}, &appInfo);
    vk::Instance instance = vk::createInstance(instanceCreateInfo);

    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty())
    {
        LogError("No GPU physical devices found");
        return -1;
    }

    vk::PhysicalDevice physicalDevice = physicalDevices[0];

    vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;

    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    vk::DeviceSize totalLocalMemory = 0;
    for (const auto& heap : memoryProperties.memoryHeaps)
    {
        if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal)
        {
            totalLocalMemory += heap.size;
        }
    }
    std::cout << "Total GPU Memory: " << totalLocalMemory / (1024 * 1024) << " MB" << std::endl;

    instance.destroy();

    return static_cast<uint16_t>(totalLocalMemory / (1024 * 1024));
}

uint16_t LLama::GetGPULayer()
{
    uint16_t gpu_memory = GetGPUMemory();
    if (gpu_memory >= 24000)
    {
        return 60;
    }
    else if (gpu_memory >= 16000)
    {
        return 40;
    }
    else if (gpu_memory >= 8000)
    {
        return 20;
    }
    else if (gpu_memory >= 4000)
    {
        return 10;
    }
    else if (gpu_memory >= 2000)
    {
        return 5;
    }
    else
    {
        return 0;
    }
}

void LLama::BuildHistory(const std::vector<std::pair<std::string, std::string>>& m_history)
{
    this->history[convid_].messages.clear();
    for (const auto& it : m_history)
    {
        ChatMessage msg;
        msg.role = it.first;
        msg.content = it.second;
        this->history[convid_].messages.push_back(msg);
    }
}

LLama::LLama(const LLamaCreateInfo& data, const std::string& sysr) : llamaData(data), sys(sysr)
{
    ggml_backend_load_all();


    llama_log_set([](enum ggml_log_level level, const char* text, void*)
    {
        if (level >= GGML_LOG_LEVEL_ERROR)
        {
            LogError("{0}", text);
        }
    }, nullptr);


    llama_model_params params = llama_model_default_params();
    llama_context_params context_params = llama_context_default_params();


    context_params.n_threads = 8;
    context_params.n_ctx = llamaData.contextSize;
    context_params.n_batch = llamaData.maxTokens;
    context_params.no_perf = false;


    int gpu_layers = GetGPULayer();
    params.n_gpu_layers = gpu_layers;

    LogInfo("LLama: 尝试加载模型，GPU层数: {0}", gpu_layers);


    if (!UFile::Exists(llamaData.model))
    {
        LogError("LLama: 模型文件未找到: {0}", llamaData.model);
        return;
    }


    FILE* test = fopen(llamaData.model.c_str(), "rb");
    if (!test)
    {
        LogError("LLama: 无法打开模型文件，请检查读取权限: {0}", strerror(errno));
        return;
    }
    fclose(test);


    try
    {
        LogInfo("LLama: 开始加载模型文件 {0}", llamaData.model);
        model = llama_model_load_from_file(llamaData.model.c_str(), params);

        if (model == nullptr)
        {
            LogError("LLama: 模型加载失败！尝试降低GPU层数重新加载");

            params.n_gpu_layers = 0;
            model = llama_model_load_from_file(llamaData.model.c_str(), params);
        }

        if (model == nullptr)
        {
            LogError("LLama: 模型加载失败！尝试减小上下文大小");

            context_params.n_ctx = min(llamaData.contextSize, 2048);
            context_params.n_batch = min(llamaData.maxTokens, 512);
        }
        else
        {
            LogInfo("LLama: 模型加载成功！");
        }


        LogInfo("LLama: 开始初始化上下文");
        ctx = llama_init_from_model(model, context_params);

        if (ctx == nullptr)
        {
            LogError("LLama: 上下文初始化失败！可能是内存不足或参数设置不当");
        }
        else
        {
            LogInfo("LLama: 上下文初始化成功！");
        }
    }
    catch (const std::exception& e)
    {
        LogError("LLama: 加载模型过程中发生异常: {0}", e.what());
        return;
    }


    if (model == nullptr || ctx == nullptr)
    {
        LogError("LLama: 模型或上下文初始化失败！");
        return;
    }


    vocab = llama_model_get_vocab(model);
    llama_sampler_chain_params p = llama_sampler_chain_default_params();
    p.no_perf = true;
    smpl = llama_sampler_chain_init(p);
    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());
    formatted = std::vector<char>(llama_n_ctx(ctx));

    LogInfo("LLama: 初始化完成，上下文大小: {0}, 最大令牌数: {1}", llamaData.contextSize, llamaData.maxTokens);
}

std::string LLama::Submit(std::string prompt, size_t timeStamp, std::string role, std::string convid, float temp,
                          float top_p, uint32_t top_k, float
                          pres_pen, float freq_pen, bool async)
{
    if (!ctx)
    {
        LogError("LLama: 上下文未初始化，无法处理请求");
        return "错误: LLama上下文未初始化。";
    }

    std::lock_guard<std::mutex> lock(historyAccessMutex);
    lastFinalResponse = "";
    lastTimeStamp = timeStamp;

    if (!history.contains(convid))
    {
        LogInfo("LLama: 创建新的对话历史 ID: {0}", convid);
        history[convid].messages = std::vector<ChatMessage>();
        history[convid].messages.push_back({"system", sys});
        history[convid].prev_len = 0;
    }

    LogInfo("LLama: 开始生成响应");
    Response[timeStamp] = std::make_tuple("", false);


    const char* tmpl = llama_model_chat_template(model, nullptr);
    if (!tmpl)
    {
        LogWarn("LLama: 模型没有内置聊天模板，使用默认格式");
    }


    history[convid].messages.push_back({role, prompt});


    int new_len = llama_chat_apply_template(tmpl, history[convid].To().data(),
                                            history[convid].To().size(), true, formatted.data(), formatted.size());
    if (new_len < 0)
    {
        LogError("LLama: 应用聊天模板失败");
        std::get<0>(Response[timeStamp]) = "错误: 应用聊天模板失败";
        std::get<1>(Response[timeStamp]) = true;
        return "错误: 应用聊天模板失败";
    }


    if (new_len > (int)formatted.size())
    {
        LogInfo("LLama: 重新分配格式化缓冲区大小为 {0}", new_len);
        formatted.resize(new_len);
        new_len = llama_chat_apply_template(tmpl, history[convid].To().data(),
                                            history[convid].To().size(), true, formatted.data(),
                                            formatted.size());
    }


    prompt = std::string(formatted.begin() + history[convid].prev_len, formatted.begin() + new_len);

    {
        {
            std::lock_guard<std::mutex> stopLock(forceStopMutex);
            if (forceStop)
            {
                LogInfo("LLama: 操作被用户取消");
                std::get<0>(Response[timeStamp]) = "操作已被取消";
                std::get<1>(Response[timeStamp]) = true;
                return "操作已被取消";
            }
        }


        LogInfo("LLama: 对提示进行分词");
        int n_prompt = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, true, true);
        if (n_prompt <= 0)
        {
            LogError("LLama: 分词失败，返回的token数量为 {0}", n_prompt);
            std::get<0>(Response[timeStamp]) = "错误: 分词失败";
            std::get<1>(Response[timeStamp]) = true;
            return "错误: 分词失败";
        }

        std::vector<llama_token> prompt_tokens(n_prompt);
        if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), true,
                           true) < 0)
        {
            LogError("LLama: 对提示进行分词失败");
            std::get<0>(Response[timeStamp]) = "错误: 对提示进行分词失败";
            std::get<1>(Response[timeStamp]) = true;
            return "错误: 对提示进行分词失败";
        }

        LogInfo("LLama: 提示分词完成，共 {0} 个token", n_prompt);


        {
            std::lock_guard<std::mutex> stopLock(forceStopMutex);
            if (forceStop)
            {
                LogInfo("LLama: 操作被用户取消");
                std::get<0>(Response[timeStamp]) = "操作已被取消";
                std::get<1>(Response[timeStamp]) = true;
                return "操作已被取消";
            }
        }


        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        int n_batch = llama_n_batch(ctx);
        int error = 0;


        if (batch.n_tokens > n_batch)
        {
            LogError("LLama: token数量超出批处理大小，请减少token数量或增加maxTokens大小");
            std::get<0>(Response[timeStamp]) = "错误: token数量超出批处理大小，请减少token数量或增加maxTokens大小";
            std::get<1>(Response[timeStamp]) = true;
            error = 1;
        }

        if (error == 0)
        {
            int n_ctx = llama_n_ctx(ctx);
            int n_ctx_used = llama_kv_self_used_cells(ctx);

            if (n_ctx_used + batch.n_tokens > n_ctx)
            {
                LogError("LLama: 上下文大小超出限制，请打开新会话或重置");
                std::get<0>(Response[timeStamp]) = "错误: 上下文大小超出限制，请打开新会话或重置";
                std::get<1>(Response[timeStamp]) = true;
                error = 3;
            }
            else
            {
                LogInfo("LLama: 开始解码生成文本");


                if (llama_decode(ctx, batch))
                {
                    LogError("LLama: 解码失败");
                    std::get<0>(Response[timeStamp]) = "错误: 解码失败";
                    std::get<1>(Response[timeStamp]) = true;
                    error = 2;
                }
                else
                {
                    llama_token new_token_id;
                    while (true)
                    {
                        {
                            std::lock_guard<std::mutex> stopLock(forceStopMutex);
                            if (forceStop)
                            {
                                LogInfo("LLama: 生成被强制中断");
                                std::get<0>(Response[timeStamp]) += "\n[生成被中断]";
                                std::get<1>(Response[timeStamp]) = true;
                                error = 5;
                                break;
                            }
                        }


                        new_token_id = llama_sampler_sample(smpl, ctx, -1);


                        if (llama_vocab_is_eog(vocab, new_token_id))
                        {
                            LogInfo("LLama: 生成完成，遇到结束标记");
                            break;
                        }


                        char buf[256];
                        int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
                        if (n < 0)
                        {
                            LogError("LLama: 转换token为文本片段失败");
                            std::get<0>(Response[timeStamp]) = "错误: 转换token为文本片段失败";
                            std::get<1>(Response[timeStamp]) = true;
                            error = 4;
                            break;
                        }


                        std::string piece(buf, n);
                        std::get<0>(Response[timeStamp]) += piece;


                        batch = llama_batch_get_one(&new_token_id, 1);
                        if (llama_decode(ctx, batch))
                        {
                            LogError("LLama: 解码失败");
                            std::get<0>(Response[timeStamp]) = "错误: 解码失败";
                            std::get<1>(Response[timeStamp]) = true;
                            error = 2;
                            break;
                        }
                    }
                }
            }
        }


        if (error == 0 || error == 5)
        {
            LogInfo("LLama: 生成完成，更新聊天历史");
            history[convid].messages.push_back({"assistant", std::get<0>(Response[timeStamp])});
            history[convid].prev_len = llama_chat_apply_template(tmpl, history[convid].To().data(),
                                                                 history[convid].To().size(), false,
                                                                 nullptr, 0);
            if (history[convid].prev_len < 0)
            {
                LogError("LLama: 应用聊天模板失败");
                std::get<0>(Response[timeStamp]) = "错误: 应用聊天模板失败";
                std::get<1>(Response[timeStamp]) = true;
                return "错误: 应用聊天模板失败";
            }
        }
    }


    if (async)
        while (!std::get<0>(Response[timeStamp]).empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    else
    {
        lastFinalResponse = GetResponse(timeStamp);
    }
    std::get<1>(Response[timeStamp]) = true;
    return lastFinalResponse;
}

LLama::~LLama()
{
    try
    {
        if (model)
            llama_model_free(model);
        if (ctx)
            llama_free(ctx);
    }
    catch (...)
    {
    }
}

void LLama::Reset()
{
    history[convid_].messages.clear();
    history[convid_].messages.push_back({"system", sys.c_str()});
    history[convid_].prev_len = 0;
    Del(convid_);
    Save(convid_);
}

void LLama::Load(std::string name)
{
    try
    {
        std::lock_guard<std::mutex> lock(fileAccessMutex);
        std::stringstream buffer;
        std::ifstream session_file(ConversationPath + name + suffix);
        if (session_file.is_open())
        {
            buffer << session_file.rdbuf();
            json j = json::parse(buffer.str());
            if (j.is_array())
            {
                for (auto& it : j)
                {
                    if (it.is_object())
                    {
                        history[name].messages.push_back({
                            it["role"].get<std::string>().c_str(),
                            it["content"].get<std::string>().c_str()
                        });
                    }
                }
            }
        }
        else
        {
            LogError("ChatBot Error: Unable to load session {}.", name);
        }
    }
    catch (std::exception& e)
    {
        LogError(e.what());
        history[name].messages.clear();
        history[name].messages.push_back({"system", sys.c_str()});
    }
    convid_ = name;
    LogInfo("Bot: 加载 {0} 成功", name);
}

void LLama::Save(std::string name)
{
    json j;
    for (auto& m : history[name].messages)
    {
        j.push_back({
            {"role", m.role},
            {"content", m.content}
        });
    }
    std::ofstream session_file(ConversationPath + name + suffix);
    if (session_file.is_open())
    {
        session_file << j.dump();
        session_file.close();
        LogInfo("Bot : Save {0} successfully", name);
    }
    else
    {
        LogError("ChatBot Error: Unable to save session {0},{1}", name, ".");
    }
}

void LLama::Del(std::string name)
{
    if (remove((ConversationPath + name + suffix).c_str()) != 0)
    {
        LogError("ChatBot Error: Unable to delete session {0},{1}", name, ".");
    }
    LogInfo("Bot : 删除 {0} 成功", name);
}

void LLama::Add(std::string name)
{
    history[name].messages = std::vector<ChatMessage>();
    history[name].messages.push_back({"system", sys.c_str()});
    history[name].prev_len = 0;
    Save(name);
}

map<long long, string> LLama::GetHistory()
{
    map<long long, string> historyMap;


    auto now = std::chrono::system_clock::now();
    auto baseTime = now - std::chrono::hours(24);
    auto baseTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(baseTime.time_since_epoch()).count();


    const long long messageInterval = 300000;


    if (history.contains(convid_))
    {
        const auto& messages = history.at(convid_).messages;

        for (size_t i = 0; i < messages.size(); ++i)
        {
            try
            {
                long long currentTimestamp = baseTimestamp + (i * messageInterval);


                json messageData;
                messageData["role"] = messages[i].role;
                messageData["content"] = messages[i].content;


                historyMap[currentTimestamp] = messageData.dump();
            }
            catch (const std::exception& e)
            {
                LogError("解析历史记录第 {} 条消息时出错: {}", i, e.what());


                long long currentTimestamp = baseTimestamp + (i * messageInterval);
                json errorData;
                errorData["role"] = "system";
                errorData["content"] = "消息解析失败";
                historyMap[currentTimestamp] = errorData.dump();
            }
        }
    }

    return historyMap;
}

std::vector<std::string> LLama::GetAllConversations()
{
    std::vector<std::string> conversationList;

    try
    {
        if (std::filesystem::exists(ConversationPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(ConversationPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == suffix)
                {
                    std::string conversationName = entry.path().stem().string();
                    conversationList.push_back(conversationName);
                }
            }
        }
        else
        {
            std::filesystem::create_directories(ConversationPath);
            Add("default");
            conversationList.push_back("default");
        }
    }
    catch (const std::exception& e)
    {
        LogError("扫描LLama对话目录失败: {}", e.what());

        conversationList.push_back("default");
    }


    std::sort(conversationList.begin(), conversationList.end(), [](const std::string& a, const std::string& b)
    {
        if (a == "default") return true;
        if (b == "default") return false;
        return a < b;
    });

    return conversationList;
}

std::string LLama::sendRequest(std::string data, size_t ts)
{
    return "";
}
