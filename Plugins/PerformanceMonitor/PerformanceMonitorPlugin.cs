using Luma.SDK.Plugins;

namespace PerformanceMonitor;

public class PerformanceMonitorPlugin : EditorPlugin
{
    public override string Id => "com.luma.perfmonitor";
    public override string Name => "性能监控";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "实时性能监控插件，提供 FPS、帧时间图表和场景统计信息。";

    private PerformancePanel? _perfPanel;

    public override void OnLoad()
    {
        base.OnLoad();
        _perfPanel = RegisterPanel<PerformancePanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("性能监控"))
        {
            if (ImGui.MenuItem(_perfPanel?.IsVisible == true ? "隐藏性能面板" : "显示性能面板"))
            {
                if (_perfPanel != null)
                    _perfPanel.IsVisible = !_perfPanel.IsVisible;
            }
            ImGui.Separator();
            if (ImGui.MenuItem("重置统计"))
            {
                _perfPanel?.ResetStats();
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "窗口")
        {
            if (ImGui.MenuItem("性能监控面板"))
            {
                if (_perfPanel != null)
                    _perfPanel.IsVisible = true;
            }
        }
    }
}

public class PerformancePanel : EditorPanel
{
    public override string Title => "性能监控";

    private const int FrameHistorySize = 60;

    private float _currentFps;
    private float _avgFps;
    private float _minFps = float.MaxValue;
    private float _maxFps;
    private int _frameCount;
    private float _fpsAccumulator;
    private float _fpsUpdateTimer;
    private const float FpsUpdateInterval = 0.25f;

    private readonly float[] _frameTimes = new float[FrameHistorySize];
    private int _frameIndex;

    public override void OnUpdate(float deltaTime)
    {
        if (deltaTime <= 0) return;

        _frameTimes[_frameIndex] = deltaTime;
        _frameIndex = (_frameIndex + 1) % FrameHistorySize;

        _frameCount++;
        _fpsAccumulator += deltaTime;
        _fpsUpdateTimer += deltaTime;

        if (_fpsUpdateTimer >= FpsUpdateInterval)
        {
            _currentFps = 1.0f / deltaTime;
            _avgFps = _frameCount / _fpsAccumulator;
            if (_currentFps > 0)
            {
                if (_currentFps < _minFps) _minFps = _currentFps;
                if (_currentFps > _maxFps) _maxFps = _currentFps;
            }
            _fpsUpdateTimer = 0;
        }
    }

    public void ResetStats()
    {
        _currentFps = 0;
        _avgFps = 0;
        _minFps = float.MaxValue;
        _maxFps = 0;
        _frameCount = 0;
        _fpsAccumulator = 0;
        _fpsUpdateTimer = 0;
        _frameIndex = 0;
        Array.Clear(_frameTimes, 0, _frameTimes.Length);
        Log.Info("[性能监控] 统计数据已重置");
    }

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("PerfTabs"))
        {
            if (ImGui.BeginTabItem("FPS"))
            {
                DrawFpsTab();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("帧时间"))
            {
                DrawFrameTimeTab();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("场景统计"))
            {
                DrawSceneStatsTab();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("编辑器状态"))
            {
                DrawEditorStateTab();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawFpsTab()
    {
        ImGui.Text("帧率统计");
        ImGui.Separator();

        float r, g, b;
        GetFpsColor(_currentFps, out r, out g, out b);
        ImGui.TextColored(r, g, b, 1.0f, $"当前 FPS: {_currentFps:F1}");
        ImGui.Text($"平均 FPS: {_avgFps:F1}");

        float displayMin = _minFps == float.MaxValue ? 0 : _minFps;
        ImGui.Text($"最小 FPS: {displayMin:F1}");
        ImGui.Text($"最大 FPS: {_maxFps:F1}");

        ImGui.Separator();
        float fpsRatio = Math.Clamp(_currentFps / 120.0f, 0, 1);
        ImGui.ProgressBar(fpsRatio, $"{_currentFps:F0} FPS");

        ImGui.Separator();
        ImGui.Text($"总帧数: {_frameCount}");

        ImGui.Separator();
        if (ImGui.Button("重置统计"))
        {
            ResetStats();
        }
    }

    private void DrawFrameTimeTab()
    {
        ImGui.Text("帧时间图表（最近 60 帧）");
        ImGui.Separator();

        float maxFrameTime = 0;
        for (int i = 0; i < FrameHistorySize; i++)
        {
            if (_frameTimes[i] > maxFrameTime)
                maxFrameTime = _frameTimes[i];
        }
        if (maxFrameTime <= 0) maxFrameTime = 0.033f;

        if (ImGui.BeginTable("FrameTimeTable", 2))
        {
            ImGui.TableSetupColumn("帧");
            ImGui.TableSetupColumn("时间");
            ImGui.TableHeadersRow();

            for (int i = 0; i < FrameHistorySize; i++)
            {
                int idx = (_frameIndex + i) % FrameHistorySize;
                float ft = _frameTimes[idx];
                if (ft <= 0) continue;

                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                ImGui.Text($"{i + 1}");
                ImGui.TableNextColumn();
                float ratio = Math.Clamp(ft / maxFrameTime, 0, 1);
                ImGui.ProgressBar(ratio, $"{ft * 1000:F1} ms");
            }
            ImGui.EndTable();
        }
    }

    private void DrawSceneStatsTab()
    {
        ImGui.Text("场景统计信息");
        ImGui.Separator();

        if (ImGui.BeginTable("SceneStatsTable", 2))
        {
            ImGui.TableSetupColumn("项目");
            ImGui.TableSetupColumn("值");
            ImGui.TableHeadersRow();

            ImGui.TableNextRow();
            ImGui.TableNextColumn();
            ImGui.Text("实体数量");
            ImGui.TableNextColumn();
            ImGui.Text($"{LumaAPI.GetEntityCount()}");

            ImGui.TableNextRow();
            ImGui.TableNextColumn();
            ImGui.Text("资源数量");
            ImGui.TableNextColumn();
            ImGui.Text($"{LumaAPI.GetAssetCount()}");

            ImGui.TableNextRow();
            ImGui.TableNextColumn();
            ImGui.Text("当前场景");
            ImGui.TableNextColumn();
            ImGui.Text($"{LumaAPI.GetCurrentSceneName()}");

            ImGui.EndTable();
        }

        ImGui.Separator();
        if (Project.IsLoaded)
        {
            ImGui.Text($"项目: {Project.Name}");
            ImGui.Text($"资产目录: {Project.AssetsPath}");
        }
        else
        {
            ImGui.TextDisabled("未加载项目");
        }
    }

    private void DrawEditorStateTab()
    {
        ImGui.Text("编辑器状态");
        ImGui.Separator();

        if (ImGui.CollapsingHeader("运行模式"))
        {
            if (Editor.IsPlaying)
                ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "● 播放模式");
            else
                ImGui.TextColored(0.3f, 0.5f, 1.0f, 1.0f, "● 编辑模式");

            ImGui.Text($"编辑器模式: {(Editor.IsEditorMode ? "是" : "否")}");
            ImGui.Text($"正在播放: {(Editor.IsPlaying ? "是" : "否")}");
        }

        ImGui.Separator();
        if (ImGui.CollapsingHeader("选中实体"))
        {
            int selectedCount = Editor.SelectedEntityCount;
            ImGui.Text($"选中数量: {selectedCount}");

            if (selectedCount > 0)
            {
                if (ImGui.BeginTable("SelectedEntities", 2))
                {
                    ImGui.TableSetupColumn("名称");
                    ImGui.TableSetupColumn("GUID");
                    ImGui.TableHeadersRow();

                    int displayCount = Math.Min(selectedCount, 10);
                    for (int i = 0; i < displayCount; i++)
                    {
                        ImGui.TableNextRow();
                        ImGui.TableNextColumn();
                        ImGui.Text(Editor.GetSelectedEntityName(i));
                        ImGui.TableNextColumn();
                        ImGui.TextDisabled($"{Editor.GetSelectedEntityGuid(i)}");
                    }
                    ImGui.EndTable();
                }

                if (selectedCount > 10)
                    ImGui.TextDisabled($"... 还有 {selectedCount - 10} 个");
            }
        }
    }

    private static void GetFpsColor(float fps, out float r, out float g, out float b)
    {
        if (fps >= 55)      { r = 0.2f; g = 0.8f; b = 0.2f; }
        else if (fps >= 30) { r = 1.0f; g = 0.8f; b = 0.0f; }
        else                { r = 1.0f; g = 0.2f; b = 0.2f; }
    }
}
