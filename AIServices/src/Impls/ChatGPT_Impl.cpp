#include "Impls/ChatGPT_Impl.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    DString* dstr = static_cast<DString*>(userp);
    size_t totalSize = size * nmemb;
    std::string dataChunk(static_cast<char*>(contents), totalSize);
    ChatBot* chatBot = static_cast<ChatBot*>(dstr->instance);


    if (chatBot && chatBot->forceStop.load())
    {
        return 0;
    }


    dstr->response->append(dataChunk);


    {
        std::lock_guard<std::mutex> lock(dstr->mtx);
        dstr->str1->append(dataChunk);
    }


    std::string buffer;
    {
        std::lock_guard<std::mutex> lock(dstr->mtx);
        buffer = *dstr->str1;
    }


    std::string processed;
    std::string remainingBuffer;
    size_t processedLength = 0;


    size_t currentPos = 0;
    size_t nextPos = 0;

    while ((nextPos = buffer.find("data:", currentPos)) != std::string::npos)
    {
        size_t endPos = buffer.find("data:", nextPos + 5);
        if (endPos == std::string::npos)
        {
            endPos = buffer.length();
        }


        std::string dataBlock = buffer.substr(nextPos, endPos - nextPos);


        if (dataBlock.find("[DONE]") != std::string::npos)
        {
            currentPos = endPos;
            continue;
        }


        size_t jsonStart = dataBlock.find('{');
        if (jsonStart != std::string::npos)
        {
            std::string jsonStr = dataBlock.substr(jsonStart);


            try
            {
                json jsonData = json::parse(jsonStr);


                if (jsonData.contains("choices") && !jsonData["choices"].empty())
                {
                    auto& choices = jsonData["choices"];

                    if (choices[0].contains("delta") &&
                        choices[0]["delta"].contains("content") &&
                        !choices[0]["delta"]["content"].is_null())
                    {
                        std::string content = choices[0]["delta"]["content"].get<std::string>();
                        processed += content;
                    }
                }
            }
            catch (...)
            {
                if (endPos != buffer.length())
                {
                    currentPos = endPos;
                    continue;
                }


                remainingBuffer = buffer.substr(nextPos);
                break;
            }
        }


        currentPos = endPos;
        processedLength = endPos;
    }


    {
        std::lock_guard<std::mutex> lock(dstr->mtx);
        if (processedLength > 0)
        {
            if (processedLength < buffer.length())
            {
                *dstr->str1 = buffer.substr(processedLength);
            }
            else
            {
                dstr->str1->clear();
            }
        }


        if (!processed.empty())
        {
            dstr->str2->append(processed);
        }
    }

    return totalSize;
}

ChatGPT::ChatGPT(std::string systemrole)
{
    if (!systemrole.empty())
        defaultJson["content"] = systemrole;
    else
        defaultJson["content"] = sys;
    defaultJson["role"] = "system";


    if (!UDirectory::Exists(ConversationPath))
    {
        UDirectory::Create(ConversationPath);
        Add("default");
    }
}

map<long long, string> ChatGPT::GetHistory()
{
    map<long long, string> historyMap;


    auto now = std::chrono::system_clock::now();
    auto baseTime = now - std::chrono::hours(24);
    auto baseTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(baseTime.time_since_epoch()).count();


    const long long messageInterval = 300000;

    for (size_t i = 0; i < history.size(); ++i)
    {
        try
        {
            long long currentTimestamp = baseTimestamp + (i * messageInterval);


            json messageData;
            if (history[i].contains("role") && history[i].contains("content"))
            {
                messageData["role"] = history[i]["role"];
                messageData["content"] = history[i]["content"];


                historyMap[currentTimestamp] = messageData.dump();
            }
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

    return historyMap;
}

std::string ChatGPT::Stamp2Time(long long timestamp)
{
    time_t tick = (time_t)(timestamp / 1000);
    tm tm;
    char s[40];
    tm = *localtime(&tick);
    strftime(s, sizeof(s), "%Y-%m-%d", &tm);
    std::string str(s);
    return str;
}

ChatGPT::ChatGPT(const OpenAIBotCreateInfo& chat_data, std::string systemrole) : chat_data_(chat_data)
{
    if (!systemrole.empty())
        defaultJson["content"] = systemrole;
    else
        defaultJson["content"] = sys;
    defaultJson["role"] = "system";

    if (!UDirectory::Exists(ConversationPath))
    {
        UDirectory::Create(ConversationPath);
        Add("default");
    }
}

std::string ChatGPT::sendRequest(std::string data, size_t ts)
{
    try
    {
        int retry_count = 0;
        while (retry_count < 3)
        {
            {
                std::lock_guard<std::mutex> stopLock(forceStopMutex);
                if (forceStop)
                {
                    std::get<0>(Response[ts]) = "操作已被取消";
                    std::get<1>(Response[ts]) = true;
                    return "操作已被取消";
                }
            }

            try
            {
                LogInfo("ChatBot: Post request...");

                std::string url = "";
                if (!chat_data_.useWebProxy)
                {
                    url = "https://api.openai.com/v1/chat/completions";
                }
                else
                {
                    url = chat_data_._endPoint;
                }


                CURL* curl;
                CURLcode res;
                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                headers = curl_slist_append(headers, ("Authorization: Bearer " + chat_data_.api_key).c_str());
                headers = curl_slist_append(headers, ("api-key: " + chat_data_.api_key).c_str());
                headers = curl_slist_append(headers, "Transfer-Encoding: chunked");
                curl = curl_easy_init();
                if (curl)
                {
                    curl_easy_setopt(curl, CURLOPT_URL, (url).c_str());
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);


                    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
                    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);


                    DString dstr;
                    dstr.str1 = new string();
                    dstr.str2 = &std::get<0>(Response[ts]);
                    dstr.response = new string();
                    dstr.instance = this;
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dstr);


                    if (!chat_data_.useWebProxy && !chat_data_.proxy.empty())
                    {
                        curl_easy_setopt(curl, CURLOPT_PROXY, chat_data_.proxy.c_str());
                    }
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);


                    res = curl_easy_perform(curl);


                    if (res == CURLE_ABORTED_BY_CALLBACK || (res == CURLE_WRITE_ERROR && forceStop))
                    {
                        LogInfo("ChatBot: Request canceled by user");
                        std::get<0>(Response[ts]) = std::get<0>(Response[ts]) + "\n[生成被中断]";
                        std::get<1>(Response[ts]) = true;


                        curl_easy_cleanup(curl);
                        curl_slist_free_all(headers);
                        delete dstr.str1;
                        delete dstr.response;

                        return std::get<0>(Response[ts]);
                    }
                    else if (res != CURLE_OK)
                    {
                        LogError("ChatBot Error: Request failed with error code {}", std::to_string(res));
                        retry_count++;


                        std::lock_guard<std::mutex> stopLock(forceStopMutex);
                        if (forceStop)
                        {
                            std::get<0>(Response[ts]) = "操作已被取消";
                            std::get<1>(Response[ts]) = true;


                            curl_easy_cleanup(curl);
                            curl_slist_free_all(headers);
                            delete dstr.str1;
                            delete dstr.response;

                            return "操作已被取消";
                        }
                    }
                    else
                    {
                        curl_easy_cleanup(curl);
                        curl_slist_free_all(headers);


                        delete dstr.str1;
                        delete dstr.response;


                        std::cout << std::get<0>(Response[ts]) << std::endl;
                        return std::get<0>(Response[ts]);
                    }
                }
            }
            catch (std::exception& e)
            {
                LogError("ChatBot Error: {}", std::string(e.what()));
                retry_count++;


                std::lock_guard<std::mutex> stopLock(forceStopMutex);
                if (forceStop)
                {
                    std::get<0>(Response[ts]) = "操作已被取消";
                    std::get<1>(Response[ts]) = true;
                    return "操作已被取消";
                }
            }
        }
        LogError("ChatBot Error: Request failed after three retries.");
    }
    catch (exception& e)
    {
        LogError(e.what());
    };
    return "";
}

void ChatGPT::BuildHistory(const std::vector<std::pair<std::string, std::string>>& history)
{
    this->history.clear();
    for (const auto& it : history)
    {
        json ask;
        ask["content"] = it.second;
        ask["role"] = it.first;
        this->history.emplace_back(ask);
    }
}

std::vector<std::string> ChatGPT::GetAllConversations()
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
            UDirectory::Create(ConversationPath);
            Add("default");
            conversationList.push_back("default");
        }
    }
    catch (const std::exception& e)
    {
        LogError("扫描ChatGPT对话目录失败: {}", e.what());

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

std::string ChatGPT::Submit(std::string prompt, size_t timeStamp, std::string role, std::string convid, float temp,
                            float top_p, uint32_t top_k, float pres_pen, float freq_pen, bool async)
{
    try
    {
        {
            std::lock_guard<std::mutex> stopLock(forceStopMutex);
            if (forceStop)
            {
                std::get<0>(Response[timeStamp]) = "操作已被取消";
                std::get<1>(Response[timeStamp]) = true;
                return "操作已被取消";
            }
        }
        lastTimeStamp = timeStamp;
        lastFinalResponse = "";
        json ask;
        ask["content"] = prompt;
        ask["role"] = role;
        convid_ = convid;
        if (Conversation.find(convid) == Conversation.end())
        {
            std::lock_guard<std::mutex> lock(historyAccessMutex);
            history.push_back(defaultJson);

            Conversation.insert({convid_, history});
        }
        history.emplace_back(ask);
        Conversation[convid_] = history;
        std::string data = "{\n"
            "  \"model\": \"" + chat_data_.model + "\",\n"
            "  \"stream\": true,\n"
            "  \"temperature\":" + std::to_string(temp) + ",\n"
            "  \"top_k\":" + std::to_string(top_k) + ",\n"
            "  \"top_p\":" + std::to_string(top_p) + ",\n"
            "  \"presence_penalty\":" + std::to_string(pres_pen) + ",\n"
            "  \"frequency_penalty\":" + std::to_string(freq_pen) + ",\n"
            "  \"messages\": " +
            Conversation[convid_].dump()
            + "\n}\n";
        Response[timeStamp] = std::make_tuple("", false);
        std::string res = sendRequest(data, timeStamp);


        {
            std::lock_guard<std::mutex> stopLock(forceStopMutex);
            if (forceStop && !res.empty() && res != "操作已被取消")
            {
                std::get<1>(Response[timeStamp]) = true;
                LogInfo("ChatBot: Post canceled but partial result saved");
                return res;
            }
        }


        if (!res.empty() && res != "操作已被取消")
        {
            std::get<1>(Response[timeStamp]) = true;
            LogInfo("ChatBot: Post finished");
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
        json response;
        response["content"] = lastFinalResponse;
        response["role"] = "assistant";
        history.emplace_back(response);
        std::get<1>(Response[timeStamp]) = true;
        return lastFinalResponse;
    }
    catch (exception& e)
    {
        LogError(e.what());
        std::get<0>(Response[timeStamp]) = "发生错误: " + std::string(e.what());
        std::get<1>(Response[timeStamp]) = true;
        return lastFinalResponse;
    }
}

void ChatGPT::Reset()
{
    history.clear();
    history.push_back(defaultJson);
    Conversation[convid_] = history;
    Del(convid_);
    Save(convid_);
}

void ChatGPT::Load(std::string name)
{
    std::lock_guard<std::mutex> lock(fileAccessMutex);
    std::stringstream buffer;
    std::ifstream session_file(ConversationPath + name + suffix);
    if (session_file.is_open())
    {
        buffer << session_file.rdbuf();
        history = json::parse(buffer.str());
        convid_ = name;
        Conversation[name] = history;
    }
    else
    {
        LogError("ChatBot Error: Unable to load session {}.", name);
    }
    LogInfo("Bot : 加载 {0} 成功", name);
}

void ChatGPT::Save(std::string name)
{
    std::ofstream session_file(ConversationPath + name + suffix);

    if (session_file.is_open())
    {
        session_file << history.dump();
        session_file.close();
        LogInfo("Bot : Save {0} successfully", name);
    }
    else
    {
        LogError("ChatBot Error: Unable to save session {0},{1}", name, ".");
    }
}

void ChatGPT::Del(std::string name)
{
    if (remove((ConversationPath + name + suffix).c_str()) != 0)
    {
        LogError("ChatBot Error: Unable to delete session {0},{1}", name, ".");
    }
    LogInfo("Bot : 删除 {0} 成功", name);
}

void ChatGPT::Add(std::string name)
{
    history.clear();
    history.emplace_back(defaultJson);
    Save(name);
}
