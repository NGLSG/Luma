#include "Impls/CustomRule_Impl.h"

#include <regex>

#include "Configure.h"
static ResponseRole ROLE;
static vector<string> RESPONSE_PATH;
static const string MD = "${MODEL}";
static const string API_KEY = "${API_KEY}";

std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

void resolveChainedVariables(std::vector<CustomVariable>& vars)
{
    if (vars.empty())
    {
        return;
    }

    int max_iterations = vars.size() * vars.size();
    int current_iteration = 0;
    bool changed_in_pass;

    do
    {
        changed_in_pass = false;
        current_iteration++;

        for (auto& target_var : vars)
        {
            std::string original_value = target_var.value;
            std::regex placeholder_regex(R"(\$\{([\w_]+)\})");
            std::smatch match;
            std::string temp_value = target_var.value;

            std::string current_processing_value = target_var.value;
            std::string last_iteration_value;

            do
            {
                last_iteration_value = current_processing_value;
                std::string search_value = current_processing_value;
                std::string new_value_for_target;
                size_t last_pos = 0;

                while (std::regex_search(search_value, match, placeholder_regex))
                {
                    std::string placeholder = match[0].str();
                    std::string var_name_to_find = match[1].str();

                    new_value_for_target += match.prefix().str();

                    bool found_source = false;
                    for (const auto& source_var : vars)
                    {
                        if (source_var.name == var_name_to_find)
                        {
                            if (&source_var != &target_var || source_var.value != original_value)
                            {
                                new_value_for_target += source_var.value;
                                found_source = true;
                            }
                            break;
                        }
                    }
                    if (!found_source)
                    {
                        new_value_for_target += placeholder;
                    }

                    search_value = match.suffix().str();
                    last_pos += match.prefix().length() + placeholder.length();
                }
                new_value_for_target += search_value;
                current_processing_value = new_value_for_target;

                if (current_processing_value != last_iteration_value)
                {
                    if (target_var.value != current_processing_value)
                    {
                        target_var.value = current_processing_value;
                        changed_in_pass = true;
                    }
                }
                else
                {
                    break;
                }
            }
            while (true);
        }

        if (current_iteration > max_iterations)
        {
            break;
        }
    }
    while (changed_in_pass);
}

void replaceVariable(const std::string& varName, std::string& text, const std::string& value)
{
    std::string var = "${" + varName + "}";
    auto p = text.find(var);
    if (p != std::string::npos)
    {
        text.replace(p, var.length(), value);
    }
}


std::string ExtractContentFromJson(const json& jsonData, const std::vector<std::string>& path)
{
    const json* current = &jsonData;


    for (size_t i = 0; i < path.size(); i++)
    {
        const auto& pathItem = path[i];


        if (current->is_array())
        {
            bool isIndex = true;
            size_t index = 0;
            try
            {
                index = std::stoul(pathItem);
            }
            catch (...)
            {
                isIndex = false;
            }

            if (isIndex)
            {
                if (index < current->size())
                {
                    current = &(*current)[index];
                }
                else
                {
                    return "";
                }
            }
            else
            {
                if (current->size() > 0)
                {
                    current = &(*current)[0];


                    i--;
                }
                else
                {
                    return "";
                }
            }
        }
        else if (current->is_object() && current->contains(pathItem) && !(*current)[pathItem].is_null())
        {
            current = &(*current)[pathItem];
        }
        else
        {
            return "";
        }
    }


    if (current->is_array() && current->size() > 0)
    {
        current = &(*current)[0];
    }


    if (current->is_string())
    {
        return current->get<std::string>();
    }
    else
    {
        return "";
    }
}

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
    size_t processedLength = 0;


    if (ROLE.suffix.empty())
    {
        try
        {
            if (!buffer.empty())
            {
                if (!ROLE.stopFlag.empty() && buffer.find(ROLE.stopFlag) != std::string::npos)
                {
                    size_t flagPos = buffer.find(ROLE.stopFlag);
                    processedLength = flagPos + ROLE.stopFlag.length();
                }
                else
                {
                    json jsonData = json::parse(buffer);


                    if (jsonData.is_array())
                    {
                        for (const auto& item : jsonData)
                        {
                            std::string extractedContent = ExtractContentFromJson(item, RESPONSE_PATH);
                            if (!extractedContent.empty())
                            {
                                processed += extractedContent;
                            }
                        }
                    }
                    else
                    {
                        std::string extractedContent = ExtractContentFromJson(jsonData, RESPONSE_PATH);
                        if (!extractedContent.empty())
                        {
                            processed += extractedContent;
                        }
                    }


                    processedLength = buffer.length();
                }
            }
        }
        catch (json::parse_error&)
        {
            processedLength = 0;
        }
    }
    else
    {
        size_t currentPos = 0;
        size_t nextPos = 0;

        while ((nextPos = buffer.find(ROLE.suffix, currentPos)) != std::string::npos)
        {
            size_t endPos = buffer.find(ROLE.suffix, nextPos + ROLE.suffix.length());
            if (endPos == std::string::npos)
            {
                endPos = buffer.length();
            }


            std::string dataBlock = buffer.substr(nextPos, endPos - nextPos);


            if (!ROLE.stopFlag.empty() && dataBlock.find(ROLE.stopFlag) != std::string::npos)
            {
                currentPos = endPos;
                processedLength = endPos;
                continue;
            }


            size_t jsonStart = dataBlock.find('{');
            if (jsonStart != std::string::npos)
            {
                std::string jsonStr = dataBlock.substr(jsonStart);

                try
                {
                    json jsonData = json::parse(jsonStr);


                    std::string extractedContent = ExtractContentFromJson(jsonData, RESPONSE_PATH);
                    if (!extractedContent.empty())
                    {
                        processed += extractedContent;
                    }
                }
                catch (...)
                {
                    if (endPos == buffer.length())
                    {
                        break;
                    }
                }
            }


            currentPos = endPos;
            processedLength = endPos;
        }
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

static std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

void JsonPathBuilder::addValueAtPath(json& jsonObj, const vector<string>& path, const string& value)
{
    if (path.empty()) return;

    json* current = &jsonObj;
    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        const string& segment = path[i];
        bool isArrayIndex = !segment.empty() &&
            std::all_of(segment.begin(), segment.end(), [](char c) { return isdigit(c); });

        if (isArrayIndex)
        {
            size_t index = std::stoul(segment);
            if (!current->is_array())
            {
                (*current) = json::array();
            }
            while (current->size() <= index)
            {
                current->push_back(json::object());
            }

            current = &(*current)[index];
        }
        else
        {
            if (!current->contains(segment))
            {
                bool nextIsArray = (i < path.size() - 2) &&
                    !path[i + 1].empty() &&
                    std::all_of(path[i + 1].begin(), path[i + 1].end(), [](char c) { return isdigit(c); });

                (*current)[segment] = nextIsArray ? json::array() : json::object();
            }
            current = &(*current)[segment];
        }
    }

    if (!path.empty())
    {
        try
        {
            (*current)[path.back()] = json::parse(value);
        }
        catch (const json::parse_error&)
        {
            (*current)[path.back()] = value;
        }
    }
}

void JsonPathBuilder::addPath(const string& pathStr, const string& value)
{
    vector<string> pathParts = split(pathStr, '/');
    erase_if(pathParts, [](const string& s) { return s.empty(); });
    addValueAtPath(rootJson, pathParts, value);
}

json JsonPathBuilder::getJson()
{
    return rootJson;
}

CustomRule_Impl::CustomRule_Impl(const CustomRule& data, std::string systemrole) : CustomRuleData(data)
{
    resolveChainedVariables(CustomRuleData.vars);
    paths = split(CustomRuleData.promptRole.prompt.path, '/');
    paths2 = split(CustomRuleData.promptRole.role.path, '/');
    RESPONSE_PATH = split(CustomRuleData.responseRole.content, '/');
    ROLE = CustomRuleData.responseRole;
    JsonPathBuilder builder;


    builder.addPath(CustomRuleData.promptRole.prompt.path, "${PROMPT}");


    builder.addPath(CustomRuleData.promptRole.role.path, "${ROLE}");

    templateJson = builder.getJson();


    if (CustomRuleData.supportSystemRole && !CustomRuleData.roles["system"].empty())
    {
        auto js = templateJson;

        js = buildRequest(systemrole, CustomRuleData.roles["system"]);

        SystemPrompt.push_back(js);
    }
    else
    {
        auto js = templateJson;
        std::string data1 = js.dump();

        js = buildRequest("Yes I am here to help you.", CustomRuleData.roles["assistant"]);
        auto js2 = templateJson;

        js2 = buildRequest(systemrole, CustomRuleData.roles["user"]);
        SystemPrompt.push_back(js2);
        SystemPrompt.push_back(js);
    }
    if (!UDirectory::Exists(ConversationPath))
    {
        UDirectory::Create(ConversationPath);
        Add("default");
    }
}

std::string CustomRule_Impl::sendRequest(std::string data, size_t ts)
{
    try
    {
        int retry_count = 0;
        while (retry_count < 3)
        {
            std::string url = CustomRuleData.apiPath;
            if (url.find(MD) != std::string::npos)
                url.replace(url.find(MD), MD.size(), CustomRuleData.model);
            if (CustomRuleData.apiKeyRole.role == "URL" && url.find(API_KEY) != std::string::npos)
                url.replace(url.find(API_KEY), API_KEY.size(), CustomRuleData.apiKeyRole.key);
            for (auto& it : CustomRuleData.vars)
            {
                replaceVariable(it.name, url, it.value);
            }


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
                CURLcode res;
                struct curl_slist* headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                for (const auto& [key, value] : CustomRuleData.headers)
                {
                    std::string v = value;
                    for (const auto& it : CustomRuleData.vars)
                    {
                        if (v.find(it.name) != std::string::npos)
                        {
                            v.replace(v.find(it.name), it.name.size(), it.value);
                        }
                    }
                    headers = curl_slist_append(headers, (key + ": " + v).c_str());
                }
                if (CustomRuleData.apiKeyRole.role == "HEADERS")
                {
                    headers = curl_slist_append(
                        headers, (CustomRuleData.apiKeyRole.header + CustomRuleData.apiKeyRole.key).c_str());
                }

                headers = curl_slist_append(headers, "Transfer-Encoding: chunked");
                CURL* curl = curl_easy_init();
                if (curl)
                {
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
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
                    lastRawResponse = *dstr.response;


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

json CustomRule_Impl::buildRequest(const std::string& prompt, const std::string& role)
{
    auto js = templateJson;


    auto setValueAtPath = [](json& j, const std::vector<std::string>& path, const json& value)
    {
        if (path.empty()) return;

        json* current = &j;
        for (size_t i = 0; i < path.size() - 1; ++i)
        {
            const auto& segment = path[i];

            if (current->is_array())
            {
                if (segment.find_first_not_of("0123456789") == std::string::npos)
                {
                    size_t index = std::stoul(segment);
                    if (index >= current->size())
                    {
                        current->push_back(json::object());
                    }
                    current = &(*current)[index];
                }
                else
                {
                    *current = json::object();
                    (*current)[segment] = json::object();
                    current = &(*current)[segment];
                }
            }
            else
            {
                if (!current->contains(segment))
                {
                    (*current)[segment] = json::object();
                }
                current = &(*current)[segment];
            }
        }

        if (!path.empty())
        {
            (*current)[path.back()] = value;
        }
    };


    if (!paths2.empty())
    {
        setValueAtPath(js, paths2, role);
    }


    if (!paths.empty())
    {
        setValueAtPath(js, paths, prompt);
    }

    return js;
}

std::string CustomRule_Impl::Submit(std::string prompt, size_t timeStamp, std::string role, std::string convid,
                                    float temp, float top_p, uint32_t top_k, float pres_pen, float freq_pen, bool async)
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
        json ask = buildRequest(prompt, CustomRuleData.roles[role]);

        convid_ = convid;
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
        std::string d = Conversation[convid].dump();
        std::string data;
        JsonPathBuilder builder;


        for (auto& param : CustomRuleData.params)
        {
            if (param.content == MD)
                param.content = CustomRuleData.model;
            for (auto& it : CustomRuleData.vars)
            {
                replaceVariable(it.name, param.content, it.value);
            }
            builder.addPath(param.path + "/" + param.suffix, param.content);
        }
        static const std::string VAR1 = "TOPK";
        static const std::string VAR2 = "TEMP";
        static const std::string VAR3 = "TOPP";
        static const std::string VAR4 = "PRES";
        static const std::string VAR5 = "FREQ";
        for (auto& must : CustomRuleData.extraMust)
        {
            replaceVariable(VAR1, must.content, std::to_string(top_k));
            replaceVariable(VAR2, must.content, std::to_string(temp));
            replaceVariable(VAR3, must.content, std::to_string(top_p));
            replaceVariable(VAR4, must.content, std::to_string(pres_pen));
            replaceVariable(VAR5, must.content, std::to_string(freq_pen));
            builder.addPath(must.path + "/" + must.suffix, must.content);
        }

        builder.addPath(CustomRuleData.promptRole.prompt.suffix, Conversation[convid].dump());

        json resultJson = builder.getJson();
        data += resultJson.dump();
        LogInfo("转发数据: {0}", data);

        if (!data.empty() && data.back() == ',')
        {
            data.pop_back();
        }
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
            if (lastFinalResponse.empty())
            {
                lastFinalResponse = lastRawResponse;
            }
        }
        json response = templateJson;
        response[paths.front()].back() = lastFinalResponse;
        response[paths2.front()].back() = CustomRuleData.roles["assistant"];

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

void CustomRule_Impl::Reset()
{
    history.clear();
    Conversation[convid_] = history;
}

void CustomRule_Impl::Load(std::string name)
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
        LogError("CustomRule_Impl Error: Unable to load session {}.", name);
    }
    LogInfo("Bot: 加载 {0} 成功", name);
}

void CustomRule_Impl::Save(std::string name)
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
        LogError("CustomRule_Impl Error: Unable to save session {0},{1}", name, ".");
    }
}

void CustomRule_Impl::Del(std::string name)
{
    if (remove((ConversationPath + name + suffix).c_str()) != 0)
    {
        LogError("CustomRule_Impl Error: Unable to delete session {0},{1}", name, ".");
    }
    LogInfo("Bot : 删除 {0} 成功", name);
}

void CustomRule_Impl::Add(std::string name)
{
    history.clear();
    Save(name);
}

map<long long, string> CustomRule_Impl::GetHistory()
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


            std::string role = "";
            std::string content = "";


            if (!paths2.empty())
            {
                json* current = &history[i];
                for (size_t j = 0; j < paths2.size() - 1; ++j)
                {
                    if (current->contains(paths2[j]))
                    {
                        current = &(*current)[paths2[j]];
                    }
                    else
                    {
                        current = nullptr;
                        break;
                    }
                }

                if (current && current->contains(paths2.back()))
                {
                    std::string rawRole = (*current)[paths2.back()].get<std::string>();


                    if (rawRole == CustomRuleData.roles["user"])
                    {
                        role = "user";
                    }
                    else if (rawRole == CustomRuleData.roles["assistant"])
                    {
                        role = "assistant";
                    }
                    else if (rawRole == CustomRuleData.roles["system"])
                    {
                        role = "system";
                    }
                    else
                    {
                        role = "unknown";
                    }
                }
            }


            if (!paths.empty())
            {
                json* current = &history[i];
                for (size_t j = 0; j < paths.size() - 1; ++j)
                {
                    if (current->contains(paths[j]))
                    {
                        current = &(*current)[paths[j]];
                    }
                    else
                    {
                        current = nullptr;
                        break;
                    }
                }

                if (current && current->contains(paths.back()))
                {
                    content = (*current)[paths.back()].get<std::string>();
                }
            }


            messageData["role"] = role;
            messageData["content"] = content;


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

    return historyMap;
}

std::vector<std::string> CustomRule_Impl::GetAllConversations()
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
        LogError("扫描自定义规则对话目录失败: {}", e.what());

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

void CustomRule_Impl::BuildHistory(const std::vector<std::pair<std::string, std::string>>& history)
{
    this->history.clear();
    for (const auto& it : history)
    {
        json js = templateJson;
        if (it.first == "system" && !CustomRuleData.supportSystemRole)
        {
            auto j1 = buildRequest(it.second, CustomRuleData.roles["user"]);
            auto j2 = buildRequest("Yes,i know that", CustomRuleData.roles["assistant"]);
            this->history.push_back(j1);
            this->history.push_back(j2);
        }
        else
        {
            this->history.push_back(buildRequest(it.second, CustomRuleData.roles[it.first]));
        }
    }
    std::string data = this->history.dump();
}
