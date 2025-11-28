#ifndef PLUGINMANAGERPANEL_H
#define PLUGINMANAGERPANEL_H

#include "IEditorPanel.h"
#include <string>
#include <filesystem>
#include <future>
#include <atomic>

struct SDL_Window;

/**
 * @brief 插件管理面板
 */
class PluginManagerPanel : public IEditorPanel
{
public:
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Shutdown() override;
    const char* GetPanelName() const override { return "插件管理"; }

    // 文件对话框回调
    void OnImportFileSelected(const std::filesystem::path& path);
    void OnPublishDirSelected(const std::filesystem::path& path);
    void OnPluginProjectDirSelected(const std::filesystem::path& path);

private:
    void drawPluginList();
    void drawPluginDetails(int selectedIndex);
    void drawImportSection();
    void drawPublishPopup();
    void doPublishAsync(const std::filesystem::path& pluginDir, const std::filesystem::path& outputFile);
    SDL_Window* getSDLWindow() const;

    int m_selectedPluginIndex = -1;
    char m_importPathBuffer[512] = {0};
    bool m_showImportDialog = false;

    // 发布配置
    char m_publishOutputDir[512] = {0};
    char m_publishPluginDir[512] = {0};
    std::string m_publishPluginId;

    // 异步发布
    std::atomic<bool> m_isPublishing{false};
    std::future<bool> m_publishFuture;
    std::string m_publishStatusMessage;
};

#endif // PLUGINMANAGERPANEL_H
