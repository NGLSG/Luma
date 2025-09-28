#ifndef AIPANEL_H
#define AIPANEL_H

#include "IEditorPanel.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <unordered_map>

#include "AITool.h"
#include "imgui.h"
#include "imgui_markdown.h"

#include "ChatBot.h"
#include "Configure.h"


/**
 * @brief AI助手面板，提供与AI聊天、配置AI模型和工具的功能。
 */
class AIPanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数，初始化AI助手面板。
     */
    AIPanel();
    /**
     * @brief 析构函数，清理AI助手面板资源。
     */
    ~AIPanel() override;

    /**
     * @brief 初始化面板。
     * @param context 编辑器上下文。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新面板逻辑。
     * @param deltaTime 帧时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制面板UI。
     */
    void Draw() override;
    /**
     * @brief 关闭面板，释放资源。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板名称。
     * @return 面板名称字符串。
     */
    const char* GetPanelName() const override { return "AI 助手"; }

private:
    /**
     * @brief 表示一条聊天消息。
     */
    struct ChatMessage
    {
        std::string role; ///< 消息发送者的角色（例如"user"或"assistant"）。
        std::string content; ///< 消息内容。
        long long timestamp; ///< 消息的时间戳。
    };

    /**
     * @brief 表示一个可用的AI模型。
     */
    struct AvailableModel
    {
        std::string displayName; ///< 模型的显示名称。
        std::string providerName; ///< 模型提供商的名称。
        std::string modelName; ///< 模型的内部名称。
        std::string botKey; ///< 关联的聊天机器人键。
    };

    /**
     * @brief 绘制聊天面板。
     */
    void drawChatPanel();
    /**
     * @brief 绘制设置面板。
     */
    void drawSettingsPanel();
    /**
     * @brief 绘制会话侧边栏。
     */
    void drawConversationSidebar();
    /**
     * @brief 初始化所有聊天机器人。
     */
    void initializeBots();
    /**
     * @brief 提交用户消息。
     */
    void submitMessage();
    /**
     * @brief 保存配置。
     */
    void saveConfiguration();
    /**
     * @brief 加载配置。
     */
    void loadConfiguration();
    /**
     * @brief 加载指定名称的会话。
     * @param name 会话名称。
     */
    void loadConversation(const std::string& name);
    /**
     * @brief 加载会话列表。
     */
    void loadConversationList();
    /**
     * @brief 重置所有设置到默认值。
     */
    void resetToDefaults();
    /**
     * @brief 检查OpenAI提供商配置是否有效。
     * @param config OpenAI机器人创建信息。
     * @return 如果配置有效则返回true，否则返回false。
     */
    bool isProviderConfigValid(const OpenAIBotCreateInfo& config) const;
    /**
     * @brief 检查Claude API提供商配置是否有效。
     * @param config Claude API创建信息。
     * @return 如果配置有效则返回true，否则返回false。
     */
    bool isProviderConfigValid(const ClaudeAPICreateInfo& config) const;
    /**
     * @brief 检查Gemini提供商配置是否有效。
     * @param config Gemini机器人创建信息。
     * @return 如果配置有效则返回true，否则返回false。
     */
    bool isProviderConfigValid(const GeminiBotCreateInfo& config) const;
    /**
     * @brief 检查GPT-Like提供商配置是否有效。
     * @param config GPT-Like机器人创建信息。
     * @return 如果配置有效则返回true，否则返回false。
     */
    bool isProviderConfigValid(const GPTLikeCreateInfo& config) const;
    /**
     * @brief 绘制支持模型列表编辑器。
     * @param supportedModels 支持的模型列表。
     * @param id UI元素的唯一标识符。
     */
    void drawSupportedModelsEditor(std::vector<std::string>& supportedModels, const char* id);
    /**
     * @brief 绘制Llama配置编辑器。
     * @param llamaConfig Llama创建信息。
     * @param id UI元素的唯一标识符。
     */
    void drawLlamaConfigEditor(LLamaCreateInfo& llamaConfig, const char* id);
    /**
     * @brief 绘制自定义规则管理界面。
     */
    void drawCustomRulesManagement();
    /**
     * @brief 绘制规则编辑器弹窗。
     */
    void drawRuleEditorPopup();
    /**
     * @brief 绘制HTTP头编辑器。
     * @param headers HTTP头键值对。
     * @param id UI元素的唯一标识符。
     */
    void drawHeadersEditor(std::unordered_map<std::string, std::string>& headers, const char* id);
    /**
     * @brief 绘制自定义变量编辑器。
     * @param variables 自定义变量列表。
     * @param id UI元素的唯一标识符。
     */
    void drawVariablesEditor(std::vector<CustomVariable>& variables, const char* id);
    /**
     * @brief 模型管理器：批量添加/选择可用模型，并可从下拉选择覆盖当前 model。
     * @param model 绑定的默认模型字段。
     * @param supportedModels 可用模型列表。
     * @param id UI唯一ID前缀。
     */
    void drawModelManager(std::string& model, std::vector<std::string>& supportedModels, const char* id);
    /**
     * @brief 创建一个默认规则。
     * @return 默认的自定义规则。
     */
    CustomRule createDefaultRule();
    /**
     * @brief 创建一个预设规则。
     * @param presetName 预设规则的名称。
     * @return 预设的自定义规则。
     */
    CustomRule createPresetRule(const std::string& presetName);
    /**
     * @brief 初始化AI工具。
     */
    void initializeAITools();
    /**
     * @brief 处理AI响应中的工具调用。
     * @param aiResponse AI的原始响应文本。
     */
    void processToolCalls(const std::string& aiResponse);
    /**
     * @brief 同步并保存聊天历史。
     */
    void synchronizeAndSaveHistory();

    /**
     * @brief 初始化Markdown渲染器。
     */
    void initializeMarkdown();

    Configure m_config; ///< 面板的配置对象。

    std::vector<ChatMessage> m_messages; ///< 当前会话的聊天消息列表。
    char m_inputBuffer[8192] = {0}; ///< 用户输入缓冲区。
    bool m_isWaitingForResponse = false; ///< 指示是否正在等待AI响应。
    uint64_t m_lastRequestTimestamp = 0; ///< 上次请求的时间戳。
    bool m_scrollToBottom = false; ///< 指示是否需要滚动到聊天底部。
    std::string m_streamBuffer; ///< AI流式响应的缓冲区。

    std::map<std::string, std::unique_ptr<ChatBot>> m_bots; ///< 存储所有聊天机器人的映射。
    std::vector<AvailableModel> m_availableModels; ///< 可用的AI模型列表。
    int m_selectedModelIndex = -1; ///< 当前选定模型的索引。
    std::string m_currentBotKey; ///< 当前使用的机器人键。

    std::string m_currentConversation = "default"; ///< 当前会话的名称。
    std::vector<std::string> m_conversationList; ///< 会话列表。
    char m_newConversationNameBuffer[64] = {0}; ///< 新会话名称输入缓冲区。

    /**
     * @brief 定义面板的视图模式。
     */
    enum class View { Chat, Settings };

    View m_currentView = View::Chat; ///< 当前的视图模式（聊天或设置）。

    bool m_showRuleEditor = false; ///< 指示是否显示规则编辑器。
    int m_editingRuleIndex = -1; ///< 正在编辑的规则索引。
    int m_selectedRuleIndex = -1; ///< 当前选中的规则索引。
    CustomRule m_tempRule; ///< 临时规则对象，用于编辑。

    char m_derivedRuleName[64] = {0}; ///< 派生规则名称缓冲区。
    char m_derivedApiKey[256] = {0}; ///< 派生API密钥缓冲区。
    char m_derivedApiPath[512] = {0}; ///< 派生API路径缓冲区。
    char m_derivedModel[128] = {0}; ///< 派生模型名称缓冲区。
    std::vector<std::string> m_derivedSupportedModels; ///< 派生支持模型列表。

    char m_tempHeaderKey[128] = {0}; ///< 临时HTTP头键缓冲区。
    char m_tempHeaderValue[256] = {0}; ///< 临时HTTP头值缓冲区。
    char m_tempVarName[64] = {0}; ///< 临时变量名缓冲区。
    char m_tempVarValue[256] = {0}; ///< 临时变量值缓冲区。
    std::string m_systemPrompt; ///< 系统提示。

    Guid m_targetedGuid; ///< 目标GUID。
    std::string m_targetedObjectName; ///< 目标对象名称。

    ImGui::MarkdownConfig m_markdownConfig; ///< Markdown渲染配置。
    size_t m_toolCallMessageIndex = -1; ///< 工具调用消息的索引。

    /**
     * @brief 过滤掉不需要的标签。
     * @param rawText 原始文本。
     * @return 过滤后的文本。
     */
    std::string filterUnintendTags(const std::string& rawText);

    // ===== 权限与工具调用审批 =====
    enum class PermissionLevel { Chat = 0, Agent = 1, AgentFull = 2 };
    PermissionLevel m_permissionLevel = PermissionLevel::Chat; ///< 当前权限等级
    bool m_showToolApprovalModal = false; ///< 是否显示工具调用审批弹窗
    bool m_hasPendingToolCalls = false; ///< 是否有待审批的工具调用
    nlohmann::json m_pendingToolCalls; ///< 待审批的工具调用（{"tool_calls": [...] }）
    void drawToolApprovalPopup(); ///< 绘制并处理工具调用审批弹窗
    void executePendingToolCalls(); ///< 执行已审批的工具调用
    void denyPendingToolCalls(const std::string& reason); ///< 拒绝工具调用并反馈给AI
};

#endif
