#ifndef EDITOR_H
#define EDITOR_H
#include <memory>
#include <vector>
#include "ApplicationBase.h"
#include "Editor/EditorContext.h"
#include "Editor/IEditorPanel.h"
class ImGuiRenderer;
class SceneRenderer;
class RuntimeScene;
struct UIDrawData;
class LUMA_API Editor final : public ApplicationBase
{
public:
    static Editor* GetInstance()
    {
        return s_instance;
    }
    Editor(ApplicationConfig config);
    bool checkDotNetEnvironment();
    ~Editor() override;
    void RequestFocusInHierarchy(const Guid& guid);
    void RequestFocusInBrowser(const Guid& guid);
    void CreateNewProject();
    void CreateNewPluginProject();
    void OpenProject();
    void LoadProject(const std::filesystem::path& projectPath);
    void CreateNewProjectAtPath(const std::filesystem::path& projectPath);
    void CreatePluginProjectAtPath(const std::filesystem::path& projectPath);
    void SetPendingProjectPath(const std::filesystem::path& path) { m_pendingProjectPath = path; }
    const std::filesystem::path& GetPendingProjectPath() const { return m_pendingProjectPath; }
    IEditorPanel* GetPanelByName(const std::string& name);
    PlatformWindow* GetPlatWindow();
    EditorContext& GetEditorContext() { return m_editorContext; }
protected:
    void InitializeDerived() override;
    void Update(float deltaTime) override;
    void Render() override;
    void ShutdownDerived() override;
private:
    void initializeEditorContext(); 
    void initializePanels(); 
    void registerPopups(); 
    void loadStartupScene(); 
    void drawAddComponentPopupContent(); 
    void drawFileConflictPopupContent(); 
    void updateUps(); 
    void updateFps();
private:
    EditorContext m_editorContext; 
    std::unique_ptr<UIDrawData> m_uiCallbacks; 
    std::vector<std::unique_ptr<IEditorPanel>> m_panels; 
    std::unique_ptr<ImGuiRenderer> m_imguiRenderer; 
    std::unique_ptr<SceneRenderer> m_sceneRenderer; 
    std::filesystem::path m_pendingProjectPath; 
    inline static Editor* s_instance; 
};
#endif
