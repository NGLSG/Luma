#include "Impls/Claude_Impl.h"

size_t ClaudeWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
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


    try
    {
        json jsonData = json::parse(dataChunk);


        if (jsonData.contains("content") && jsonData["content"].is_array() && !jsonData["content"].empty())
        {
            for (const auto& contentItem : jsonData["content"])
            {
                if (contentItem.contains("type") && contentItem["type"] == "text" &&
                    contentItem.contains("text") && contentItem["text"].is_string())
                {
                    std::string text = contentItem["text"].get<std::string>();
                    dstr->str2->append(text);
                }
            }
        }
    }
    catch (...)
    {
        dstr->str1->append(dataChunk);
    }

    return totalSize;
}


Claude::Claude(std::string systemrole)
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


Claude::Claude(const ClaudeAPICreateInfo& claude_data, std::string systemrole) : claude_data_(claude_data)
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

map<long long, string> Claude::GetHistory()
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

std::vector<std::string> Claude::GetAllConversations()
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
        LogError("扫描Claude对话目录失败: {}", e.what());

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


std::string Claude::Stamp2Time(long long timestamp)
{
    time_t tick = (time_t)(timestamp / 1000);
    tm tm;
    char s[40];
    tm = *localtime(&tick);
    strftime(s, sizeof(s), "%Y-%m-%d", &tm);
    std::string str(s);
    return str;
}


bool Claude::IsSaved()
{
    return LastHistory == history;
}


long long Claude::getCurrentTimestamp()
{
    auto currentTime = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();
}


long long Claude::getTimestampBefore(const int daysBefore)
{
    auto currentTime = std::chrono::system_clock::now();
    auto days = std::chrono::hours(24 * daysBefore);
    auto targetTime = currentTime - days;
    return std::chrono::duration_cast<std::chrono::milliseconds>(targetTime.time_since_epoch()).count();
}

void Claude::BuildHistory(const std::vector<std::pair<std::string, std::string>>& history)
{
    this->history.clear();
    this->history.push_back(defaultJson);
    for (const auto& it : history)
    {
        json ask = defaultJson;
        ask["content"] = it.first;
        ask["role"] = it.second;
        this->history.push_back(ask);
    }
}


std::string Claude::sendRequest(std::string data, size_t ts)
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
                LogInfo("ClaudeBot: 发送请求...");


                std::string url = claude_data_._endPoint;


                CURL* curl;
                CURLcode res;
                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                headers = curl_slist_append(headers, ("x-api-key: " + claude_data_.apiKey).c_str());


                headers = curl_slist_append(headers, ("anthropic-version: " + claude_data_.apiVersion).c_str());

                curl = curl_easy_init();
                if (curl)
                {
                    curl_easy_setopt(curl, CURLOPT_URL, (url).c_str());
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
                    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ClaudeWriteCallback);


                    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
                    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);


                    DString dstr;
                    dstr.str1 = new string();
                    dstr.str2 = &std::get<0>(Response[ts]);
                    dstr.response = new string();
                    dstr.instance = this;
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dstr);


                    res = curl_easy_perform(curl);


                    if (res == CURLE_ABORTED_BY_CALLBACK || (res == CURLE_WRITE_ERROR && forceStop))
                    {
                        LogInfo("ClaudeBot: 请求被用户取消");
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
                        LogError("ClaudeBot 错误: 请求失败，错误代码 {}", std::to_string(res));
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


                        if (!dstr.response->empty())
                        {
                            try
                            {
                                json responseJson = json::parse(*dstr.response);


                                if (responseJson.contains("content") && responseJson["content"].is_array() && !
                                    responseJson["content"].empty())
                                {
                                    std::string fullContent;

                                    for (const auto& contentItem : responseJson["content"])
                                    {
                                        if (contentItem.contains("type") && contentItem["type"] == "text" &&
                                            contentItem.contains("text") && contentItem["text"].is_string())
                                        {
                                            fullContent += contentItem["text"].get<std::string>();
                                        }
                                    }


                                    if (!fullContent.empty())
                                    {
                                        std::get<0>(Response[ts]) = fullContent;
                                    }
                                }
                            }
                            catch (...)
                            {
                                LogWarn("ClaudeBot: 响应解析失败，使用已处理的内容");
                            }
                        }

                        delete dstr.str1;
                        delete dstr.response;


                        std::cout << std::get<0>(Response[ts]) << std::endl;
                        return std::get<0>(Response[ts]);
                    }
                }
            }
            catch (std::exception& e)
            {
                LogError("ClaudeBot 错误: {}", std::string(e.what()));
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
        LogError("ClaudeBot 错误: 三次尝试后请求仍然失败。");
    }
    catch (exception& e)
    {
        LogError(e.what());
    };
    return "";
}


std::string Claude::Submit(std::string prompt, size_t timeStamp, std::string role, std::string convid, float temp,
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
        lastFinalResponse = "";
        lastTimeStamp = timeStamp;
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
            "  \"model\": \"" + claude_data_.model + "\",\n"
            "  \"max_tokens\": 4096,\n"
            "  \"temperature\":" + std::to_string(temp) + ",\n"
            "  \"top_k\":" + std::to_string(top_k) + ",\n"
            "  \"top_p\":" + std::to_string(top_p) + ",\n"
            "  \"presence_penalty\":" + std::to_string(pres_pen) + ",\n"
            "  \"frequency_penalty\":" + std::to_string(freq_pen) + ",\n"
            "  \"messages\": " +
            Conversation[convid_].dump()
            + "}\n";

        Response[timeStamp] = std::make_tuple("", false);
        std::string res = sendRequest(data, timeStamp);


        {
            std::lock_guard<std::mutex> stopLock(forceStopMutex);
            if (forceStop && !res.empty() && res != "操作已被取消")
            {
                json response;
                response["content"] = res;
                response["role"] = "assistant";
                history.emplace_back(response);
                std::get<1>(Response[timeStamp]) = true;
                LogInfo("ClaudeBot: 生成被取消，但保留了部分结果");
                return res;
            }
        }


        if (!res.empty() && res != "操作已被取消")
        {
            json response;
            response["content"] = res;
            response["role"] = "assistant";
            history.emplace_back(response);
            std::get<1>(Response[timeStamp]) = true;
            LogInfo("ClaudeBot: 请求完成");
        }
        if (async)
            while (!std::get<0>(Response[timeStamp]).empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
        return std::get<0>(Response[timeStamp]);
    }
}


void Claude::Reset()
{
    history.clear();
    history.push_back(defaultJson);
    Conversation[convid_] = history;
    Del(convid_);
    Save(convid_);
}


void Claude::Load(std::string name)
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
        LogError("ClaudeBot 错误: 无法加载会话 {}.", name);
    }
    LogInfo("ClaudeBot: 加载 {0} 成功", name);
}


void Claude::Save(std::string name)
{
    std::ofstream session_file(ConversationPath + name + suffix);

    if (session_file.is_open())
    {
        session_file << history.dump();
        session_file.close();
        LogInfo("ClaudeBot: 保存 {0} 成功", name);
    }
    else
    {
        LogError("ClaudeBot 错误: 无法保存会话 {0},{1}", name, "。");
    }
}


void Claude::Del(std::string name)
{
    if (remove((ConversationPath + name + suffix).c_str()) != 0)
    {
        LogError("ClaudeBot 错误: 无法删除会话 {0},{1}", name, "。");
    }
    LogInfo("ClaudeBot: 删除 {0} 成功", name);
}


void Claude::Add(std::string name)
{
    history.clear();
    history.emplace_back(defaultJson);
    Save(name);
}

std::string ClaudeInSlack::Submit(std::string text, size_t timeStamp, std::string role, std::string convid, float temp,
                                  float top_p, uint32_t top_k, float pres_pen, float freq_pen, bool async)
{
    try
    {
        std::lock_guard<std::mutex> lock(historyAccessMutex);
        Response[timeStamp] = std::make_tuple("", false);
        lastFinalResponse = "";
        lastTimeStamp = timeStamp;


        CURL* curl = curl_easy_init();
        if (!curl)
        {
            LogError("CURL初始化失败");
            std::get<0>(Response[timeStamp]) = "CURL初始化失败";
            std::get<1>(Response[timeStamp]) = true;
            return "CURL初始化失败";
        }


        std::string postFields = std::string("token=") + curl_easy_escape(curl, claudeData.slackToken.c_str(), 0) +
            std::string("&channel=") + curl_easy_escape(curl, claudeData.channelID.c_str(), 0) +
            std::string("&text=") + curl_easy_escape(curl, text.c_str(), 0);


        curl_easy_setopt(curl, CURLOPT_URL, "https://slack.com/api/chat.postMessage");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());


        std::string responseData;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         [](char* contents, size_t size, size_t nmemb, std::string* userp) -> size_t
                         {
                             userp->append(contents, size * nmemb);
                             return size * nmemb;
                         });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);


        std::thread requestThread([curl, &responseData, timeStamp, this]()
        {
            CURLcode res = CURLE_OK;


            while (true)
            {
                {
                    std::lock_guard<std::mutex> stopLock(forceStopMutex);
                    if (forceStop)
                    {
                        std::get<0>(Response[timeStamp]) = "操作已被取消";
                        std::get<1>(Response[timeStamp]) = true;
                        curl_easy_cleanup(curl);
                        return;
                    }
                }


                curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100);
                res = curl_easy_perform(curl);


                if (res != CURLE_OPERATION_TIMEDOUT)
                {
                    break;
                }
            }


            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

            if (res == CURLE_OK && httpCode == 200)
            {
                try
                {
                    json response = json::parse(responseData);
                }
                catch (const std::exception& e)
                {
                    std::get<0>(Response[timeStamp]) = "解析响应失败: " + std::string(e.what());
                }
            }
            else
            {
                std::string errorMsg = "请求失败: ";
                if (res != CURLE_OK)
                {
                    errorMsg += curl_easy_strerror(res);
                }
                else
                {
                    errorMsg += "HTTP错误 " + std::to_string(httpCode);
                }
                LogError(errorMsg.c_str());
                std::get<0>(Response[timeStamp]) = errorMsg;
            }


            curl_easy_cleanup(curl);
        });


        requestThread.detach();
    }
    catch (std::exception& e)
    {
        LogError(e.what());
        std::get<0>(Response[timeStamp]) = "发生错误: " + std::string(e.what());
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

void ClaudeInSlack::Reset()
{
    Submit("请忘记上面的会话内容", GetCurrentTimestamp());
    LogInfo("Claude : 重置成功");
}

void ClaudeInSlack::Load(std::string name)
{
    LogInfo("Claude : 不支持的操作");
}

void ClaudeInSlack::Save(std::string name)
{
    LogInfo("Claude : 不支持的操作");
}

void ClaudeInSlack::Del(std::string id)
{
    LogInfo("Claude : 不支持的操作");
}

void ClaudeInSlack::Add(std::string name)
{
    LogInfo("Claude : 不支持的操作");
}

map<long long, string> ClaudeInSlack::GetHistory()
{
    try
    {
        History.clear();
        auto _ts = to_string(GetCurrentTimestamp());


        CURL* curl = curl_easy_init();
        if (!curl)
        {
            LogError("CURL初始化失败");
            return History;
        }


        std::string postFields = std::string("channel=") + curl_easy_escape(curl, claudeData.channelID.c_str(), 0) +
            std::string("&latest=") + curl_easy_escape(curl, _ts.c_str(), 0) +
            std::string("&token=") + curl_easy_escape(curl, claudeData.slackToken.c_str(), 0);


        curl_easy_setopt(curl, CURLOPT_URL, "https://slack.com/api/conversations.history");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());


        std::string responseData;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         [](char* contents, size_t size, size_t nmemb, std::string* userp) -> size_t
                         {
                             userp->append(contents, size * nmemb);
                             return size * nmemb;
                         });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);


        CURLcode res = curl_easy_perform(curl);


        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            json j = json::parse(responseData);
            if (j["ok"].get<bool>())
            {
                for (auto& message : j["messages"])
                {
                    if (message["bot_profile"]["name"] == "Claude")
                    {
                        long long ts = (long long)(atof(message["ts"].get<string>().c_str()) * 1000);
                        std::string text = message["blocks"][0]["elements"][0]["elements"][0]["text"].get<string>();

                        History[ts] = text;
                    }
                }
            }
        }
        else
        {
            LogError("获取历史记录失败: {}", std::string(curl_easy_strerror(res)));
        }
    }
    catch (const std::exception& e)
    {
        LogError("获取历史记录失败: {}", string(e.what()));
    }
    return History;
}

std::vector<std::string> ClaudeInSlack::GetAllConversations()
{
    std::vector<std::string> conversationList;
    conversationList.push_back("default");
    return conversationList;
}
