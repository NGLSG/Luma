#ifndef AIPANEL_H
#define AIPANEL_H
#include "IEditorPanel.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>
#include "AITool.h"
#include "imgui.h"
#include "imgui_markdown.h"
#include "ChatBot.h"
#include "Configure.h"
class AIPanel : public IEditorPanel
{
public:
    AIPanel();
    ~AIPanel() override;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "AI 助手"; }
private:
    struct ChatMessage
    {
        std::string role; 
        std::string content; 
        long long timestamp; 
    };
    struct AvailableModel
    {
        std::string displayName; 
        std::string providerName; 
        std::string modelName; 
        std::string botKey; 
    };
    struct ToolCallEntry
    {
        std::string functionName; 
        nlohmann::json arguments; 
        std::optional<nlohmann::json> result; 
        std::string reason; 
    };
    struct ToolInvocationLog
    {
        int messageIndex = -1; 
        std::vector<ToolCallEntry> calls; 
        uint64_t timestamp = 0; 
        std::string rawRequest; 
    };
    void drawChatPanel();
    void drawSettingsPanel();
    void drawConversationSidebar();
    void drawRightOptionsPanel();
    void initializeBots();
    void submitMessage();
    void saveConfiguration();
    void loadConfiguration();
    void loadConversation(const std::string& name);
    void loadConversationList();
    void resetToDefaults();
    bool isProviderConfigValid(const OpenAIBotCreateInfo& config) const;
    bool isProviderConfigValid(const ClaudeAPICreateInfo& config) const;
    bool isProviderConfigValid(const GeminiBotCreateInfo& config) const;
    bool isProviderConfigValid(const GPTLikeCreateInfo& config) const;
    void drawSupportedModelsEditor(std::vector<std::string>& supportedModels, const char* id);
    void drawLlamaConfigEditor(LLamaCreateInfo& llamaConfig, const char* id);
    void drawCustomRulesManagement();
    void drawRuleEditorPopup();
    void drawHeadersEditor(std::unordered_map<std::string, std::string>& headers, const char* id);
    void drawVariablesEditor(std::vector<CustomVariable>& variables, const char* id);
    void drawModelManager(std::string& model, std::vector<std::string>& supportedModels, const char* id);
    CustomRule createDefaultRule();
    CustomRule createPresetRule(const std::string& presetName);
    void initializeAITools();
    void processToolCalls(const std::string& aiResponse);
    void synchronizeAndSaveHistory();
    void initializeMarkdown();
    Configure m_config; 
    std::vector<ChatMessage> m_messages; 
    char m_inputBuffer[8192] = {0}; 
    bool m_isWaitingForResponse = false; 
    uint64_t m_lastRequestTimestamp = 0; 
    bool m_scrollToBottom = false; 
    std::string m_streamBuffer; 
    std::vector<ToolInvocationLog> m_toolInvocationLogs; 
    int m_pendingToolLogIndex = -1; 
    std::optional<nlohmann::json> m_pendingToolResults; 
    std::map<std::string, std::unique_ptr<ChatBot>> m_bots; 
    std::vector<AvailableModel> m_availableModels; 
    int m_selectedModelIndex = -1; 
    std::string m_currentBotKey; 
    std::string m_currentConversation = "default"; 
    std::vector<std::string> m_conversationList; 
    char m_newConversationNameBuffer[64] = {0}; 
    enum class View { Chat, Settings };
    View m_currentView = View::Chat; 
    bool m_showRuleEditor = false; 
    int m_editingRuleIndex = -1; 
    int m_selectedRuleIndex = -1; 
    CustomRule m_tempRule; 
    char m_derivedRuleName[64] = {0}; 
    char m_derivedApiKey[256] = {0}; 
    char m_derivedApiPath[512] = {0}; 
    char m_derivedModel[128] = {0}; 
    std::vector<std::string> m_derivedSupportedModels; 
    char m_tempHeaderKey[128] = {0}; 
    char m_tempHeaderValue[256] = {0}; 
    char m_tempVarName[64] = {0}; 
    char m_tempVarValue[256] = {0}; 
    std::string m_systemPrompt; 
    Guid m_targetedGuid; 
    std::string m_targetedObjectName; 
    ImGui::MarkdownConfig m_markdownConfig; 
    size_t m_toolCallMessageIndex = -1; 
    std::string filterUnintendTags(const std::string& rawText);
    enum class PermissionLevel { Chat = 0, Agent = 1, AgentFull = 2 };
    PermissionLevel m_permissionLevel = PermissionLevel::Chat; 
    bool m_showToolApprovalModal = false; 
    bool m_hasPendingToolCalls = false; 
    nlohmann::json m_pendingToolCalls; 
    void drawToolApprovalPopup(); 
    void executePendingToolCalls(); 
    void denyPendingToolCalls(const std::string& reason); 
    std::string buildToolResultsMarkdown(const nlohmann::json& toolResults) const;
    bool m_enableTemperature = false; 
    bool m_enableTopP = false; 
    bool m_enableTopK = false; 
    float m_paramTemperature = 0.7f; 
    float m_paramTopP = 0.9f; 
    int m_paramTopK = 40; 
};
#endif
