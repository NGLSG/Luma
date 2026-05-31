using Luma.SDK.Plugins;
namespace SceneInspector;

public class SceneInspectorPlugin : EditorPlugin
{
    public override string Id => "com.luma.sceneinspector";
    public override string Name => "场景检查器";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "场景检查器插件，提供场景概览、实体详情、项目信息和实体搜索功能。";

    private SceneInfoPanel? _sceneInfoPanel;

    public override void OnLoad()
    {
        base.OnLoad();
        _sceneInfoPanel = RegisterPanel<SceneInfoPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("场景检查器"))
        {
            if (ImGui.MenuItem("显示场景信息面板"))
            {
                if (_sceneInfoPanel != null)
                    _sceneInfoPanel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "窗口")
        {
            if (ImGui.MenuItem("场景检查器"))
            {
                if (_sceneInfoPanel != null)
                    _sceneInfoPanel.IsVisible = true;
            }
        }
    }
}

public class SceneInfoPanel : EditorPanel
{
    public override string Title => "场景信息";

    private string _searchFilter = "";

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("SceneInfoTabs"))
        {
            if (ImGui.BeginTabItem("场景概览"))
            {
                DrawSceneOverview();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("选中实体"))
            {
                DrawSelectedEntities();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("项目信息"))
            {
                DrawProjectInfo();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawSceneOverview()
    {
        ImGui.Text("当前场景");
        ImGui.Separator();

        ImGui.Text($"场景名称: {LumaAPI.GetCurrentSceneName()}");
        ImGui.Text($"实体总数: {LumaAPI.GetEntityCount()}");
        ImGui.Text($"资源总数: {LumaAPI.GetAssetCount()}");

        ImGui.Separator();
        ImGui.Text("状态指示");
        ImGui.Separator();

        if (LumaAPI.IsEditorMode())
            ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "● 编辑器模式");
        else
            ImGui.TextColored(0.8f, 0.2f, 0.2f, 1.0f, "● 非编辑器模式");

        ImGui.SameLine();

        if (LumaAPI.IsPlaying())
            ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "● 播放中");
        else
            ImGui.TextColored(0.8f, 0.2f, 0.2f, 1.0f, "● 已停止");
    }

    private void DrawSelectedEntities()
    {
        int selectedCount = LumaAPI.GetSelectedEntityCount();
        ImGui.Text($"选中实体: {selectedCount} 个");
        ImGui.Separator();

        ImGui.Text("搜索:");
        ImGui.SameLine();
        ImGui.InputText("##EntitySearch", ref _searchFilter);

        ImGui.Separator();

        if (selectedCount == 0)
        {
            ImGui.TextDisabled("未选中任何实体");
            return;
        }

        if (ImGui.BeginTable("SelectedEntitiesTable", 3))
        {
            ImGui.TableSetupColumn("#");
            ImGui.TableSetupColumn("名称");
            ImGui.TableSetupColumn("GUID");
            ImGui.TableHeadersRow();

            for (int i = 0; i < selectedCount; i++)
            {
                string name = LumaAPI.GetSelectedEntityName(i);
                string guid = LumaAPI.GetSelectedEntityGuid(i);

                if (!string.IsNullOrEmpty(_searchFilter) &&
                    !name.Contains(_searchFilter, StringComparison.OrdinalIgnoreCase))
                    continue;

                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                ImGui.Text($"{i + 1}");
                ImGui.TableNextColumn();
                ImGui.Text(name);
                ImGui.TableNextColumn();
                ImGui.TextDisabled(guid);
            }

            ImGui.EndTable();
        }
    }

    private void DrawProjectInfo()
    {
        ImGui.Text("项目信息");
        ImGui.Separator();

        if (Project.IsLoaded)
        {
            ImGui.Text($"项目名称: {Project.Name}");
            ImGui.Text($"项目路径: {Project.Path}");
            ImGui.Text($"资产目录: {Project.AssetsPath}");
        }
        else
        {
            ImGui.TextColored(1.0f, 0.5f, 0.0f, 1.0f, "未加载项目");
        }
    }
}
