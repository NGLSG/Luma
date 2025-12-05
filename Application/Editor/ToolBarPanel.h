#ifndef TOOLBARPANEL_H
#define TOOLBARPANEL_H
#include <future>
#include <vector>
#include <array>
#include "IEditorPanel.h"
#include <string>
#include <filesystem>
enum class TargetPlatform;
class ProjectSettings;
class ToolbarPanel : public IEditorPanel
{
public:
    ToolbarPanel() = default;
    ~ToolbarPanel() override = default;
    void Initialize(EditorContext* context) override;
    void Update(float deltaTime) override;
    void Draw() override;
    void drawPackagingPopup();
    void Shutdown() override;
    const char* GetPanelName() const override { return "工具栏"; }
    void OnKeystoreSavePathChosen(const std::filesystem::path& path);
private:
    void drawMainMenuBar();
    void drawViewportMenu();
    void drawProjectMenu();
    void drawWindowMenu();
    void drawFileMenu();
    void drawEditMenu();
    void drawSettingsWindow();
    void drawPlayControls();
    void drawFpsDisplay();
    void updateFps();
    void rebuildScripts();
    bool runScriptCompilationLogic(std::string& statusMessage, const std::filesystem::path& outPath = "");
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
    void updateAndroidGradleProperties(const std::filesystem::path& platformOutputDir, const ProjectSettings& settings);
    std::filesystem::path signAndroidApk(const std::filesystem::path& unsignedApk, const ProjectSettings& settings);
    void refreshKeystoreCandidates(const std::filesystem::path& projectRoot);
    void drawKeystorePickerPopup(const std::filesystem::path& projectRoot);
    void drawCreateKeystorePopup();
    void drawCreateAliasPopup();
    SDL_Window* getSDLWindow() const;
    bool m_isPackaging = false; 
    std::string m_packagingStatus; 
    float m_packagingProgress = 0.0f; 
    bool m_isSettingsWindowVisible; 
    bool m_isCompilingScripts; 
    bool m_compilationFinished; 
    bool m_compilationSuccess; 
    std::string m_compilationStatus; 
    ListenerHandle m_CSharpScriptUpdated; 
    std::future<void> m_packagingFuture; 
    std::future<void> m_compilationFuture; 
    bool m_packagingSuccess; 
    std::filesystem::path m_lastBuildDirectory; 
    bool m_isTransitioningPlayState = false; 
    bool m_shouldOpenKeystorePicker = false;
    std::vector<std::filesystem::path> m_keystoreCandidates;
    std::array<char, 512> m_keystorePickerBuffer{};
    struct KeystorePopupState
    {
        bool openRequested = false;
        char path[512] = "";
        char storePassword[128] = "";
        char storePasswordConfirm[128] = "";
        char alias[128] = "luma_key";
        char aliasPassword[128] = "";
        char aliasPasswordConfirm[128] = "";
        std::string errorMessage;
    } m_keystorePopupState;
    struct AliasPopupState
    {
        bool openRequested = false;
        char alias[128] = "";
        char password[128] = "";
        char passwordConfirm[128] = "";
        std::string errorMessage;
    } m_aliasPopupState;
};
#endif
