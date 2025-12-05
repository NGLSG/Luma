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
enum class EditorState
{
    Editing, 
    Playing, 
    Paused 
};
enum class EditingMode
{
    Scene, 
    Prefab 
};
enum class SelectionType
{
    NA, 
    GameObject, 
    SceneCamera 
};
enum class AssetBrowserViewMode
{
    List, 
    Grid 
};
struct EditorContext
{
    EngineContext* engineContext = nullptr; 
    ImGuiRenderer* imguiRenderer = nullptr; 
    SceneRenderer* sceneRenderer = nullptr; 
    GraphicsBackend* graphicsBackend = nullptr; 
    Editor* editor = nullptr; 
    sk_sp<RuntimeScene> activeScene = nullptr; 
    std::vector<RenderPacket> renderQueue; 
    sk_sp<RuntimeScene> editingScene = nullptr; 
    sk_sp<RuntimeScene> sceneBeforePrefabEdit = nullptr; 
    std::string currentSceneName; 
    EditorState editorState = EditorState::Editing; 
    EditingMode editingMode = EditingMode::Scene; 
    Guid editingPrefabGuid; 
    SelectionType selectionType = SelectionType::NA; 
    std::vector<Guid> selectionList; 
    Guid selectionAnchor; 
    std::optional<std::vector<Data::PrefabNode>> gameObjectClipboard; 
    std::vector<Guid> gameObjectsToDelete; 
    std::vector<std::filesystem::path> selectedAssets; 
    std::filesystem::path assetBrowserSelectionAnchor; 
    Guid currentEditingAnimationClipGuid; 
    Guid currentEditingAnimationControllerGuid; 
    std::unique_ptr<DirectoryNode> assetTreeRoot; 
    DirectoryNode* currentAssetDirectory = nullptr; 
    AssetBrowserViewMode assetBrowserViewMode = AssetBrowserViewMode::Grid; 
    bool assetBrowserSortAscending = true; 
    std::filesystem::path itemToRename; 
    char renameBuffer[256] = {0}; 
    std::string componentClipboard_Type; 
    YAML::Node componentClipboard_Data; 
    std::vector<std::filesystem::path> assetClipboard; 
    Guid objectToFocusInHierarchy; 
    Guid assetToFocusInBrowser; 
    std::vector<std::string> droppedFilesQueue; 
    std::string conflictSourcePath; 
    std::string conflictDestPath; 
    std::chrono::steady_clock::time_point lastFrameTime; 
    std::chrono::steady_clock::time_point lastFpsUpdateTime; 
    int frameCount = 0; 
    float lastFps = 0.0f; 
    float renderLatency = 0.0f; 
    std::chrono::steady_clock::time_point lastUpsUpdateTime; 
    int updateCount = 0; 
    float lastUps = 0.0f; 
    float updateLatency = 0.0f; 
    float assetBrowserRefreshTimer = 0.0f; 
    const float assetBrowserRefreshInterval = 0.0f; 
    UIDrawData* uiCallbacks = nullptr; 
    bool wasSceneDirty; 
    Guid currentEditingTilesetGuid; 
    Guid currentEditingRuleTileGuid; 
    AssetHandle activeTileBrush; 
    sk_sp<RuntimeTexture> activeBrushPreviewImage; 
    SkRect activeBrushPreviewSourceRect; 
    Guid currentEditingBlueprintGuid; 
};
#endif
