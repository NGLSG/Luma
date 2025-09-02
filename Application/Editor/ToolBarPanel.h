#ifndef TOOLBARPANEL_H
#define TOOLBARPANEL_H

#include <future>

#include "IEditorPanel.h"
#include <string>
#include <filesystem>

enum class TargetPlatform;
/**
 * @brief 工具栏面板类。
 *
 * 负责显示编辑器中的工具栏，包含各种操作按钮和菜单。
 */
class ToolbarPanel : public IEditorPanel
{
public:
    /**
     * @brief 构造函数。
     */
    ToolbarPanel() = default;
    /**
     * @brief 析构函数。
     */
    ~ToolbarPanel() override = default;

    /**
     * @brief 初始化工具栏面板。
     * @param context 编辑器上下文指针。
     */
    void Initialize(EditorContext* context) override;
    /**
     * @brief 更新工具栏面板的状态。
     * @param deltaTime 距离上一帧的时间间隔。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 绘制工具栏面板。
     */
    void Draw() override;
    /**
     * @brief 绘制打包弹窗。
     */
    void drawPackagingPopup();

    /**
     * @brief 关闭工具栏面板。
     */
    void Shutdown() override;
    /**
     * @brief 获取面板名称。
     * @return 面板名称字符串。
     */
    const char* GetPanelName() const override { return "工具栏"; }

private:
    void drawMainMenuBar();
    void drawProjectMenu();
    void drawFileMenu();
    void drawEditMenu();
    void drawSettingsWindow();
    void drawPlayControls();
    void drawFpsDisplay();
    void updateFps();
    void rebuildScripts();
    bool runScriptCompilationLogic(std::string& statusMessage);
    void drawPreferencesPopup();
    void drawScriptCompilationPopup();

    void newScene();
    void saveScene();
    void play();
    void pause();
    void stop();
    void undo();
    void redo();
    void drawSaveBeforePackagingPopup();
    void packageGame();
    void handleShortcuts();
    void startPackagingProcess();
    bool runScriptCompilationLogicForPackaging(std::string& statusMessage, TargetPlatform targetPlatform);

    bool m_isPackaging = false; ///< 指示当前是否正在打包。
    std::string m_packagingStatus; ///< 打包状态信息。
    float m_packagingProgress = 0.0f; ///< 打包进度。
    bool m_isSettingsWindowVisible; ///< 指示设置窗口是否可见。
    bool m_isCompilingScripts; ///< 指示当前是否正在编译脚本。
    bool m_compilationFinished; ///< 指示脚本编译是否完成。
    bool m_compilationSuccess; ///< 指示脚本编译是否成功。
    std::string m_compilationStatus; ///< 脚本编译状态信息。
    ListenerHandle m_CSharpScriptUpdated; ///< C# 脚本更新事件的监听器句柄。
    std::future<void> m_packagingFuture; ///< 打包操作的异步结果。
    std::future<void> m_compilationFuture; ///< 脚本编译操作的异步结果。
    bool m_packagingSuccess; ///< 指示打包是否成功。
    std::filesystem::path m_lastBuildDirectory; ///< 上次构建的目录。
};

#endif