#ifndef EDITORCONTEXT_H
#define EDITORCONTEXT_H

#include <memory>
#include <chrono>
#include <optional>
#include <vector>
#include <map>
#include <filesystem>


#include <imgui.h>
#include <webgpu/webgpu_cpp.h>
#include <entt/entt.hpp>
#include <include/core/SkRefCnt.h>
#include <yaml-cpp/yaml.h>


#include "../Data/EngineContext.h"
#include "../Resources/AssetMetadata.h"
#include "../Data/PrefabData.h"
#include "../Utils/Guid.h"
#include "include/core/SkImage.h"

struct RenderPacket;
class RuntimeTexture;
class Editor;

class ImGuiRenderer;
class SceneRenderer;
class RuntimeScene;
class RuntimeGameObject;
class GraphicsBackend;
struct ComponentRegistration;
struct DirectoryNode;
struct UIDrawData;


/**
 * @brief 编辑器状态枚举。
 */
enum class EditorState
{
    Editing, ///< 编辑模式
    Playing, ///< 播放模式
    Paused ///< 暂停模式
};

/**
 * @brief 编辑模式枚举。
 */
enum class EditingMode
{
    Scene, ///< 场景编辑模式
    Prefab ///< 预制体编辑模式
};

/**
 * @brief 选择类型枚举。
 */
enum class SelectionType
{
    NA, ///< 无选择
    GameObject, ///< 游戏对象选择
    SceneCamera ///< 场景相机选择
};

/**
 * @brief 资源浏览器视图模式枚举。
 */
enum class AssetBrowserViewMode
{
    List, ///< 列表视图
    Grid ///< 网格视图
};


/**
 * @brief 编辑器上下文结构体，包含编辑器运行所需的所有核心数据和引用。
 *
 * 这个结构体汇集了引擎上下文、渲染器、当前场景、编辑器状态、选择信息、
 * 资源管理数据以及UI相关回调等，是编辑器全局状态的中心。
 */
struct EditorContext
{
    EngineContext* engineContext = nullptr; ///< 引擎上下文指针
    ImGuiRenderer* imguiRenderer = nullptr; ///< ImGui渲染器指针
    SceneRenderer* sceneRenderer = nullptr; ///< 场景渲染器指针
    GraphicsBackend* graphicsBackend = nullptr; ///< 图形后端指针
    Editor* editor = nullptr; ///< 编辑器主类指针


    sk_sp<RuntimeScene> activeScene = nullptr; ///< 当前激活的运行时场景
    std::vector<RenderPacket> renderQueue; ///< 当前帧的渲染队列
    sk_sp<RuntimeScene> editingScene = nullptr; ///< 正在编辑的运行时场景
    sk_sp<RuntimeScene> sceneBeforePrefabEdit = nullptr; ///< 进入预制体编辑前保存的场景
    std::string currentSceneName; ///< 当前场景的名称


    EditorState editorState = EditorState::Editing; ///< 当前编辑器状态
    EditingMode editingMode = EditingMode::Scene; ///< 当前编辑模式
    Guid editingPrefabGuid; ///< 正在编辑的预制体的全局唯一标识符


    SelectionType selectionType = SelectionType::NA; ///< 当前选择类型


    std::vector<Guid> selectionList; ///< 当前选中的对象列表（Guid）


    Guid selectionAnchor; ///< 选择锚点（用于多选）

    std::optional<std::vector<Data::PrefabNode>> gameObjectClipboard; ///< 游戏对象剪贴板数据
    std::vector<Guid> gameObjectsToDelete; ///< 待删除的游戏对象列表
    std::vector<std::filesystem::path> selectedAssets; ///< 选中的资源路径列表
    std::filesystem::path assetBrowserSelectionAnchor; ///< 资源浏览器中的选择锚点


    Guid currentEditingAnimationClipGuid; ///< 当前正在编辑的动画剪辑的Guid
    Guid currentEditingAnimationControllerGuid; ///< 当前正在编辑的动画控制器的Guid


    std::unique_ptr<DirectoryNode> assetTreeRoot; ///< 资源目录树的根节点
    DirectoryNode* currentAssetDirectory = nullptr; ///< 资源浏览器当前显示的目录
    AssetBrowserViewMode assetBrowserViewMode = AssetBrowserViewMode::Grid; ///< 资源浏览器视图模式
    bool assetBrowserSortAscending = true; ///< 资源浏览器排序方向（升序/降序）
    std::filesystem::path itemToRename; ///< 待重命名的文件或目录路径
    char renameBuffer[256] = {0}; ///< 重命名操作的输入缓冲区


    std::string componentClipboard_Type; ///< 组件剪贴板中的组件类型
    YAML::Node componentClipboard_Data; ///< 组件剪贴板中的组件数据
    std::vector<std::filesystem::path> assetClipboard; ///< 资源剪贴板中的资源路径列表


    Guid objectToFocusInHierarchy; ///< 层级面板中需要聚焦的游戏对象Guid
    Guid assetToFocusInBrowser; ///< 资源浏览器中需要聚焦的资源Guid


    std::vector<std::string> droppedFilesQueue; ///< 拖放文件队列
    std::string conflictSourcePath; ///< 文件冲突的源路径
    std::string conflictDestPath; ///< 文件冲突的目标路径


    std::chrono::steady_clock::time_point lastFrameTime; ///< 上一帧的时间点
    std::chrono::steady_clock::time_point lastFpsUpdateTime; ///< 上次更新FPS的时间点
    int frameCount = 0; ///< 帧计数器
    float lastFps = 0.0f; ///< 上次计算的FPS值
    float renderLatency = 0.0f; ///< 渲染延迟 (ms)

    std::chrono::steady_clock::time_point lastUpsUpdateTime; ///< 上次更新UPS的时间点
    int updateCount = 0; ///< 更新计数器
    float lastUps = 0.0f; ///< 上次计算的UPS值
    float updateLatency = 0.0f; ///< 逻辑更新延迟 (ms)

    float assetBrowserRefreshTimer = 0.0f; ///< 资源浏览器刷新计时器
    const float assetBrowserRefreshInterval = 0.0f; ///< 资源浏览器刷新间隔


    UIDrawData* uiCallbacks = nullptr; ///< UI绘制回调数据
    bool wasSceneDirty; ///< 场景是否曾被修改过
    Guid currentEditingTilesetGuid; ///< 当前正在编辑的瓦片集Guid
    Guid currentEditingRuleTileGuid; ///< 当前正在编辑的规则瓦片Guid
    AssetHandle activeTileBrush; ///< 当前激活的瓦片画刷句柄
    sk_sp<RuntimeTexture> activeBrushPreviewImage; ///< 激活画刷的预览图像
    SkRect activeBrushPreviewSourceRect; ///< 激活画刷预览图像的源矩形
    Guid currentEditingBlueprintGuid; ///< 当前正在编辑的蓝图Guid
};

#endif
