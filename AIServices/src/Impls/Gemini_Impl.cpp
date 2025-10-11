#include "Impls/Gemini_Impl.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

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

    try
    {
        auto jsonArray = json::parse(buffer);


        for (const auto& item : jsonArray)
        {
            if (item.contains("candidates") && !item["candidates"].empty())
            {
                const auto& candidates = item["candidates"];


                if (candidates[0].contains("content") &&
                    candidates[0]["content"].contains("parts") &&
                    !candidates[0]["content"]["parts"].empty() &&
                    candidates[0]["content"]["parts"][0].contains("text"))
                {
                    std::string content = candidates[0]["content"]["parts"][0]["text"].get<std::string>();
                    processed += content;
                }
            }
        }


        processedLength = buffer.length();
    }
    catch (json::parse_error&)
    {
        processedLength = 0;
    }


    {
        std::lock_guard<std::mutex> lock(dstr->mtx);
        if (processedLength > 0)
        {
            dstr->str1->clear();
        }


        if (!processed.empty())
        {
            dstr->str2->append(processed);
        }
    }

    return totalSize;
}

Gemini::Gemini(const GeminiBotCreateInfo& data, const std::string sys) : geminiData(data)
{
    json _t;

    _t["role"] = "user";
    _t["parts"] = json::array();
    _t["parts"].push_back(json::object());
    _t["parts"][0]["text"] = sys;

    json _t2;
    _t2["role"] = "model";
    _t2["parts"] = json::array();
    _t2["parts"].push_back(json::object());
    _t2["parts"][0]["text"] = "Yes I am here to help you.";
    SystemPrompt.push_back(_t);
    SystemPrompt.push_back(_t2);
}

map<long long, string> Gemini::GetHistory()
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
            if (history[i].contains("role") && history[i].contains("parts"))
            {
                std::string role = history[i]["role"];

                if (role == "model")
                {
                    messageData["role"] = "assistant";
                }
                else
                {
                    messageData["role"] = role;
                }


                if (!history[i]["parts"].empty() &&
                    history[i]["parts"][0].contains("text"))
                {
                    messageData["content"] = history[i]["parts"][0]["text"];
                }
                else
                {
                    messageData["content"] = "";
                }


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

std::vector<std::string> Gemini::GetAllConversations()
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
        LogError("扫描Gemini对话目录失败: {}", e.what());

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

std::string Gemini::GetModel()
{
    return geminiData.model;
}

std::string Gemini::sendRequest(std::string data, size_t ts)
{
    try
    {
        int retry_count = 0;
        while (retry_count < 3)
        {
            std::string endPoint = "";
            if (!geminiData._endPoint.empty() && geminiData._endPoint != "")
                endPoint = geminiData._endPoint;
            else
                endPoint = "https://generativelanguage.googleapis.com";
            std::string url = endPoint + "/v1beta/models/" + geminiData.model + ":streamGenerateContent?key="
                +
                geminiData._apiKey;

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
                CURL* curl;
                CURLcode res;
                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
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


std::string Gemini::Submit(std::string prompt, size_t timeStamp, std::string role, std::string convid, float temp,
                           float top_p, uint32_t top_k, float
                           pres_pen, float freq_pen, bool async)
{
    try
    {
        std::lock_guard<std::mutex> lock(historyAccessMutex);
        convid_ = convid;
        lastFinalResponse = "";
        lastTimeStamp = timeStamp;
        Response[timeStamp] = std::make_tuple("", false);
        json ask;
        ask["role"] = role;
        ask["parts"] = json::array();
        ask["parts"].push_back(json::object());
        ask["parts"][0]["text"] = prompt;
        if (Conversation.find(convid) == Conversation.end())
        {
            Conversation.insert({convid, history});
        }
        if (history.empty())
        {
            for (auto& i : SystemPrompt)
            {
                history.push_back(i);
            }
        }
        history.emplace_back(ask);
        Conversation[convid] = history;
        
        json payload;
        payload["contents"] = Conversation[convid];
        json genCfg = json::object();
        if (temp >= 0.0f) genCfg["temperature"] = temp;
        if (top_p >= 0.0f) genCfg["topP"] = top_p;
        if (top_k > 0u) genCfg["topK"] = top_k;
        
        if (!genCfg.empty()) payload["generationConfig"] = genCfg;
        
        auto addByPath = [](json& j, const std::string& path, const json& val)
        {
            if (path.empty()) return;
            size_t pos = 0; json* cur = &j;
            while (true)
            {
                size_t dot = path.find('.', pos);
                std::string key = path.substr(pos, dot == std::string::npos ? std::string::npos : dot - pos);
                if (dot == std::string::npos)
                {
                    (*cur)[key] = val;
                    break;
                }
                cur = &((*cur)[key]);
                pos = dot + 1;
            }
        };
        auto toJsonValue = [](const CustomVariable& v) -> json
        {
            if (v.isStr) return json(v.value);
            std::string low = v.value; std::transform(low.begin(), low.end(), low.begin(), ::tolower);
            if (low == "true") return json(true);
            if (low == "false") return json(false);
            char* endptr = nullptr; long long ival = strtoll(v.value.c_str(), &endptr, 10);
            if (endptr && *endptr == '\0') return json(ival);
            char* fend = nullptr; double dval = strtod(v.value.c_str(), &fend);
            if (fend && *fend == '\0') return json(dval);
            return json(v.value);
        };
        for (const auto& v : ChatBot::GlobalParams)
        {
            if (!v.name.empty()) addByPath(payload, v.name, toJsonValue(v));
        }
        std::string data = payload.dump();
        cout << data << std::endl;

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
        json ans;
        ans["role"] = "model";
        ans["parts"] = json::array();
        ans["parts"].push_back(json::object());
        ans["parts"][0]["text"] = lastFinalResponse;
        history.emplace_back(ans);
        Conversation[convid_] = history;
        std::get<0>(Response[timeStamp]) = lastFinalResponse;

        std::get<1>(Response[timeStamp]) = true;
        return lastFinalResponse;
    }
    catch (const std::exception& e)
    {
        LogError("获取历史记录失败: {}", std::string(e.what()));
        std::get<0>(Response[timeStamp]) = "发生错误: " + std::string(e.what());
        std::get<1>(Response[timeStamp]) = true;
        return lastFinalResponse;
    }
}

void Gemini::Reset()
{
    history.clear();
    Conversation[convid_] = history;
}

void Gemini::Load(std::string name)
{
    std::lock_guard<std::mutex> lock(fileAccessMutex);
    std::stringstream buffer;
    std::ifstream session_file(ConversationPath + name + suffix);
    if (session_file.is_open())
    {
        buffer << session_file.rdbuf();
        history = json::parse(buffer.str());
    }
    else
    {
        LogError("Gemini Error: Unable to load session {}", name);
    }
    LogInfo("Bot: 加载 {0} 成功", name);
}

void Gemini::Save(std::string name)
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
        LogError("Gemini Error: Unable to save session {0},{1}", name, ".");
    }
}

void Gemini::Del(std::string name)
{
    if (remove((ConversationPath + name + suffix).c_str()) != 0)
    {
        LogError("Gemini Error: Unable to delete session {0},{1}", name, ".");
    }
    LogInfo("Bot : 删除 {0} 成功", name);
}

void Gemini::Add(std::string name)
{
    history.clear();
    Save(name);
}

void Gemini::BuildHistory(const std::vector<std::pair<std::string, std::string>>& history)
{
    this->history.clear();

    for (const auto& it : history)
    {
        json ask;
        if (it.first == "user")
        {
            ask["role"] = "user";
        }
        else if (it.first == "assistant")
        {
            ask["role"] = "model";
        }
        else if (it.first == "system")
        {
            SystemPrompt.clear();
            ask["role"] = "user";
            ask["parts"] = json::array();
            ask["parts"].push_back(json::object());
            ask["parts"][0]["text"] = it.second;
            SystemPrompt.push_back(ask);
            json ask2;
            ask2["role"] = "model";
            ask2["parts"] = json::array();
            ask2["parts"].push_back(json::object());
            ask2["parts"][0]["text"] = "Yes I am here to help you.";
            SystemPrompt.push_back(ask2);
            for (auto& i : SystemPrompt)
            {
                this->history.push_back(i);
            }
        }
        else
        {
            ask["parts"] = json::array();
            ask["parts"].push_back(json::object());
            ask["parts"][0]["text"] = it.second;
            this->history.emplace_back(ask);
        }
    }
}
