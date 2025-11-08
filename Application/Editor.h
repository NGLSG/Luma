#ifndef EDITOR_H
#define EDITOR_H

#include <memory>
#include <vector>


#include "ApplicationBase.h"
#include "Editor/EditorContext.h"
#include "Editor/IEditorPanel.h"


// 前向声明
class ImGuiRenderer;
class SceneRenderer;
class RuntimeScene;
struct UIDrawData;


/**
 * @brief 编辑器主类，继承自ApplicationBase，负责管理编辑器的生命周期和核心功能。
 * @details 这是一个最终类，表示它是应用程序的编辑器入口点。
 */
class LUMA_API Editor final : public ApplicationBase
{
public:
    /**
     * @brief 构造函数，初始化编辑器实例。
     * @param config 应用程序的配置信息。
     */
    Editor(ApplicationConfig config);

    /**
     * @brief 检查.NET运行环境是否可用。
     * @return 如果.NET环境可用则返回true，否则返回false。
     */
    bool checkDotNetEnvironment();

    /**
     * @brief 析构函数，清理编辑器资源。
     */
    ~Editor() override;


    /**
     * @brief 请求在层级面板中聚焦到指定GUID的实体。
     * @param guid 要聚焦的实体的全局唯一标识符。
     */
    void RequestFocusInHierarchy(const Guid& guid);

    /**
     * @brief 请求在浏览器面板中聚焦到指定GUID的资源。
     * @param guid 要聚焦的资源的全局唯一标识符。
     */
    void RequestFocusInBrowser(const Guid& guid);

    /**
     * @brief 创建一个新的项目。
     */
    void CreateNewProject();

    /**
     * @brief 打开一个现有项目。
     */
    void OpenProject();

    /**
     * @brief 加载指定路径的项目。
     * @param projectPath 项目文件的路径。
     */
    void LoadProject(const std::filesystem::path& projectPath);

    /**
     * @brief 在指定路径创建一个新项目。
     * @param projectPath 新项目的创建路径。
     */
    void CreateNewProjectAtPath(const std::filesystem::path& projectPath);

    /**
     * @brief 根据名称获取编辑器面板。
     * @param name 面板的名称。
     * @return 指向指定名称面板的指针，如果未找到则可能返回nullptr。
     */
    IEditorPanel* GetPanelByName(const std::string& name);
    PlatformWindow* GetPlatWindow();

protected:
    /**
     * @brief 初始化派生类特有的组件。
     * @details 这是ApplicationBase的虚函数重写。
     */
    void InitializeDerived() override;

    /**
     * @brief 更新编辑器状态。
     * @param deltaTime 自上一帧以来的时间间隔（秒）。
     * @details 这是ApplicationBase的虚函数重写。
     */
    void Update(float deltaTime) override;

    /**
     * @brief 渲染编辑器界面和场景。
     * @details 这是ApplicationBase的虚函数重写。
     */
    void Render() override;

    /**
     * @brief 关闭派生类特有的组件。
     * @details 这是ApplicationBase的虚函数重写。
     */
    void ShutdownDerived() override;

private:
    void initializeEditorContext(); ///< 初始化编辑器上下文。
    void initializePanels();        ///< 初始化所有编辑器面板。
    void registerPopups();          ///< 注册所有弹出窗口。
    void loadStartupScene();        ///< 加载启动场景。


    void drawAddComponentPopupContent(); ///< 绘制添加组件弹出窗口的内容。
    void drawFileConflictPopupContent(); ///< 绘制文件冲突弹出窗口的内容。


    void updateUps(); ///< 更新帧率显示。
    void updateFps();

private:
    EditorContext m_editorContext; ///< 编辑器上下文，包含编辑器状态和数据。


    std::unique_ptr<UIDrawData> m_uiCallbacks; ///< UI绘制数据和回调的智能指针。


    std::vector<std::unique_ptr<IEditorPanel>> m_panels; ///< 存储所有编辑器面板的集合。


    std::unique_ptr<ImGuiRenderer> m_imguiRenderer; ///< ImGui渲染器的智能指针。
    std::unique_ptr<SceneRenderer> m_sceneRenderer; ///< 场景渲染器的智能指针。
};

#endif