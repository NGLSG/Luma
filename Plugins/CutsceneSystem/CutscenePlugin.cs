using Luma.SDK.Plugins;
namespace CutsceneSystem;

public class CutscenePlugin : EditorPlugin
{
    public override string Id => "com.luma.cutscene";
    public override string Name => "过场动画";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "时间线式过场动画编辑器与播放器";

    private CutsceneEditorPanel? _editorPanel;
    private CutscenePlayerPanel? _playerPanel;

    public override void OnLoad()
    {
        base.OnLoad();
        _editorPanel = RegisterPanel<CutsceneEditorPanel>();
        _playerPanel = RegisterPanel<CutscenePlayerPanel>();
        if (_editorPanel != null && _playerPanel != null)
            _playerPanel.SetEditorPanel(_editorPanel);
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("过场"))
        {
            if (ImGui.MenuItem("新建过场"))
            {
                if (_editorPanel != null)
                {
                    _editorPanel.IsVisible = true;
                    _editorPanel.CreateNewCutscene();
                }
            }
            if (ImGui.MenuItem("打开编辑器"))
            {
                if (_editorPanel != null)
                    _editorPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("打开播放器"))
            {
                if (_playerPanel != null)
                    _playerPanel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "窗口")
        {
            if (ImGui.MenuItem("过场编辑器"))
            {
                if (_editorPanel != null) _editorPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("过场播放器"))
            {
                if (_playerPanel != null) _playerPanel.IsVisible = true;
            }
        }
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("过场动画"))
            {
                if (_editorPanel != null) _editorPanel.IsVisible = true;
            }
        }
    }
}

public class CutsceneEditorPanel : EditorPanel
{
    public override string Title => "过场编辑器";

    private readonly CutsceneDatabase _db = new();
    private int _selectedCsIndex = -1;
    private int _selectedTrackIndex = -1;
    private CutsceneEvent? _selectedEvent;
    private string _newCsName = "NewCutscene";
    private string _newTrackName = "Track";
    private int _newTrackType;
    private string _paramKey = "";
    private string _paramValue = "";

    public CutsceneDatabase Database => _db;

    public void CreateNewCutscene()
    {
        _db.AddCutscene($"Cutscene_{_db.Cutscenes.Count + 1}");
        _selectedCsIndex = _db.Cutscenes.Count - 1;
    }

    public Cutscene? GetSelectedCutscene() => _db.GetCutscene(_selectedCsIndex);

    public override void OnGUI()
    {
        if (ImGui.BeginChild("CsLeft", 180, 0))
        {
            DrawCutsceneList();
        }
        ImGui.EndChild();

        ImGui.SameLine();

        if (ImGui.BeginChild("CsRight", 0, 0))
        {
            DrawRightPane();
        }
        ImGui.EndChild();
    }

    private void DrawCutsceneList()
    {
        ImGui.Text("过场列表");
        ImGui.Separator();

        for (int i = 0; i < _db.Cutscenes.Count; i++)
        {
            ImGui.PushID(i);
            if (ImGui.Selectable(_db.Cutscenes[i].Name, _selectedCsIndex == i))
            {
                _selectedCsIndex = i;
                _selectedTrackIndex = -1;
                _selectedEvent = null;
            }
            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.InputText("##newcs", ref _newCsName);
        ImGui.SameLine();
        if (ImGui.Button("+"))
        {
            _db.AddCutscene(_newCsName);
            _selectedCsIndex = _db.Cutscenes.Count - 1;
            _newCsName = $"Cutscene_{_db.Cutscenes.Count + 1}";
        }

        if (_selectedCsIndex >= 0 && _selectedCsIndex < _db.Cutscenes.Count)
        {
            if (ImGui.Button("删除"))
            {
                _db.Cutscenes.RemoveAt(_selectedCsIndex);
                _selectedCsIndex = Math.Min(_selectedCsIndex, _db.Cutscenes.Count - 1);
                _selectedEvent = null;
            }
        }
    }

    private void DrawRightPane()
    {
        var cs = GetSelectedCutscene();
        if (cs == null)
        {
            ImGui.Text("请选择或新建过场");
            return;
        }

        if (ImGui.BeginTabBar("CsEditorTabs"))
        {
            if (ImGui.BeginTabItem("时间线"))
            {
                DrawTimeline(cs);
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("事件编辑"))
            {
                DrawEventEditor(cs);
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("设置"))
            {
                DrawSettings(cs);
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawTimeline(Cutscene cs)
    {
        ImGui.Text($"过场: {cs.Name} ({cs.TotalDuration:F1}s)");
        ImGui.Separator();

        for (int t = 0; t < cs.Tracks.Count; t++)
        {
            var track = cs.Tracks[t];
            ImGui.PushID(t + 5000);

            bool muted = track.IsMuted;
            ImGui.Checkbox("##mute", ref muted);
            track.IsMuted = muted;
            ImGui.SameLine();

            var (r, g, b) = GetTrackColor(track.Type);
            ImGui.TextColored(r, g, b, 1.0f, $"[{track.Type}] {track.Name}");

            if (ImGui.Selectable($"##trk{t}", _selectedTrackIndex == t))
                _selectedTrackIndex = t;

            ImGui.Indent();
            for (int e = 0; e < track.Events.Count; e++)
            {
                var evt = track.Events[e];
                ImGui.PushID(e + 6000);
                string label = $"{evt.StartTime:F1}-{evt.EndTime:F1}s [{evt.Type}]";
                if (ImGui.Selectable(label, _selectedEvent == evt))
                    _selectedEvent = evt;
                ImGui.PopID();
            }
            ImGui.Unindent();

            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.InputText("轨道名", ref _newTrackName);
        string[] typeNames = { "Camera", "Entity", "Audio", "Dialogue", "Wait", "Custom" };
        ImGui.SliderInt("类型", ref _newTrackType, 0, 5);
        ImGui.SameLine();
        ImGui.Text(typeNames[Math.Clamp(_newTrackType, 0, 5)]);

        if (ImGui.Button("添加轨道"))
        {
            cs.AddTrack(_newTrackName, (TrackType)_newTrackType);
        }
        ImGui.SameLine();
        if (_selectedTrackIndex >= 0 && _selectedTrackIndex < cs.Tracks.Count && ImGui.Button("删除轨道"))
        {
            cs.Tracks.RemoveAt(_selectedTrackIndex);
            _selectedTrackIndex = -1;
            _selectedEvent = null;
        }
    }

    private void DrawEventEditor(Cutscene cs)
    {
        if (_selectedTrackIndex >= 0 && _selectedTrackIndex < cs.Tracks.Count)
        {
            var track = cs.Tracks[_selectedTrackIndex];
            ImGui.Text($"轨道: {track.Name}");
            if (ImGui.Button("添加事件"))
            {
                var evt = cs.AddEvent(track, 0, 1.0f);
                _selectedEvent = evt;
            }
        }

        ImGui.Separator();

        if (_selectedEvent == null)
        {
            ImGui.TextDisabled("未选中事件");
            return;
        }

        var ev = _selectedEvent;
        ImGui.Text($"事件 ID: {ev.Id}");

        float start = ev.StartTime;
        if (ImGui.SliderFloat("开始时间", ref start, 0, cs.TotalDuration))
            ev.StartTime = start;

        float dur = ev.Duration;
        if (ImGui.SliderFloat("持续时间", ref dur, 0.1f, cs.TotalDuration))
            ev.Duration = dur;

        string target = ev.TargetId;
        if (ImGui.InputText("目标ID", ref target))
            ev.TargetId = target;

        int ease = (int)ev.Ease;
        string[] easeNames = { "Linear", "EaseIn", "EaseOut", "EaseInOut" };
        if (ImGui.SliderInt("缓动", ref ease, 0, 3))
            ev.Ease = (EaseType)ease;
        ImGui.SameLine();
        ImGui.Text(easeNames[Math.Clamp(ease, 0, 3)]);

        ImGui.Separator();
        ImGui.Text("参数:");
        if (ImGui.BeginTable("ParamTable", 3))
        {
            ImGui.TableSetupColumn("Key");
            ImGui.TableSetupColumn("Value");
            ImGui.TableSetupColumn("操作");
            ImGui.TableHeadersRow();

            string? removeKey = null;
            foreach (var kv in ev.Parameters)
            {
                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                ImGui.Text(kv.Key);
                ImGui.TableNextColumn();
                ImGui.Text(kv.Value);
                ImGui.TableNextColumn();
                ImGui.PushID(kv.Key);
                if (ImGui.SmallButton("X"))
                    removeKey = kv.Key;
                ImGui.PopID();
            }
            ImGui.EndTable();

            if (removeKey != null)
                ev.Parameters.Remove(removeKey);
        }

        ImGui.InputText("Key", ref _paramKey);
        ImGui.SameLine();
        ImGui.InputText("Val", ref _paramValue);
        ImGui.SameLine();
        if (ImGui.Button("添加") && _paramKey.Length > 0)
        {
            ev.Parameters[_paramKey] = _paramValue;
            _paramKey = "";
            _paramValue = "";
        }

        ImGui.Separator();
        if (ImGui.Button("删除事件"))
        {
            foreach (var track in cs.Tracks)
                track.Events.Remove(ev);
            _selectedEvent = null;
        }
    }

    private void DrawSettings(Cutscene cs)
    {
        ImGui.Text("过场设置");
        ImGui.Separator();

        string name = cs.Name;
        if (ImGui.InputText("名称", ref name))
            cs.Name = name;

        float dur = cs.TotalDuration;
        if (ImGui.SliderFloat("总时长(s)", ref dur, 1.0f, 300.0f))
            cs.TotalDuration = dur;

        ImGui.Separator();
        ImGui.Text($"轨道数: {cs.Tracks.Count}");
        int totalEvents = 0;
        foreach (var t in cs.Tracks) totalEvents += t.Events.Count;
        ImGui.Text($"事件数: {totalEvents}");
    }

    private static (float r, float g, float b) GetTrackColor(TrackType type) => type switch
    {
        TrackType.Camera => (0.3f, 0.7f, 1.0f),
        TrackType.Entity => (0.5f, 0.9f, 0.4f),
        TrackType.Audio => (1.0f, 0.7f, 0.3f),
        TrackType.Dialogue => (0.9f, 0.5f, 0.9f),
        TrackType.Wait => (0.6f, 0.6f, 0.6f),
        TrackType.Custom => (0.8f, 0.8f, 0.3f),
        _ => (1f, 1f, 1f)
    };
}

public class CutscenePlayerPanel : EditorPanel
{
    public override string Title => "过场播放器";

    private readonly CutscenePlayer _player = new();
    private CutsceneEditorPanel? _editorPanel;
    private readonly List<string> _activeEventLog = new();

    public void SetEditorPanel(CutsceneEditorPanel panel)
    {
        _editorPanel = panel;
        _player.OnEventTriggered += evt =>
        {
            _activeEventLog.Add($"[{evt.Type}] target={evt.TargetId} t={evt.StartTime:F2}");
        };
    }

    public override void OnGUI()
    {
        DrawControls();
        ImGui.Separator();

        ImGui.Text($"进度: {_player.CurrentTime:F2} / {(_player.CurrentCutscene?.TotalDuration ?? 0):F1}s");
        ImGui.ProgressBar(_player.Progress, $"{(_player.Progress * 100):F0}%");

        ImGui.Separator();

        if (ImGui.BeginTabBar("PlayerTabs"))
        {
            if (ImGui.BeginTabItem("当前事件"))
            {
                DrawActiveEvents();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("执行日志"))
            {
                DrawLog();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawControls()
    {
        float speed = _player.PlaybackSpeed;
        ImGui.SliderFloat("播放速度", ref speed, 0.1f, 3.0f);
        _player.PlaybackSpeed = speed;

        if (!_player.IsPlaying)
        {
            if (ImGui.Button("播放"))
            {
                var cs = _editorPanel?.GetSelectedCutscene();
                if (cs != null)
                {
                    _activeEventLog.Clear();
                    _player.Play(cs);
                }
            }
        }
        else
        {
            if (_player.IsPaused)
            {
                if (ImGui.Button("继续")) _player.Resume();
            }
            else
            {
                if (ImGui.Button("暂停")) _player.Pause();
            }
            ImGui.SameLine();
            if (ImGui.Button("停止")) _player.Stop();
        }

        ImGui.SameLine();
        if (ImGui.Button("重播"))
        {
            var cs = _editorPanel?.GetSelectedCutscene();
            if (cs != null)
            {
                _activeEventLog.Clear();
                _player.Play(cs);
            }
        }
    }

    private void DrawActiveEvents()
    {
        if (_player.CurrentCutscene == null)
        {
            ImGui.TextDisabled("无过场");
            return;
        }

        var events = _player.CurrentCutscene.GetEventsAtTime(_player.CurrentTime);
        ImGui.Text($"当前活跃事件: {events.Count}");
        ImGui.Separator();

        foreach (var evt in events)
        {
            var (r, g, b) = evt.Type switch
            {
                TrackType.Camera => (0.3f, 0.7f, 1.0f),
                TrackType.Audio => (1.0f, 0.7f, 0.3f),
                TrackType.Dialogue => (0.9f, 0.5f, 0.9f),
                _ => (0.7f, 0.9f, 0.5f)
            };
            ImGui.TextColored(r, g, b, 1.0f, $"[{evt.Type}] {evt.TargetId} ({evt.StartTime:F1}-{evt.EndTime:F1}s)");
        }
    }

    private void DrawLog()
    {
        ImGui.Text($"日志 ({_player.Log.Count} + {_activeEventLog.Count} 条)");
        if (ImGui.Button("清空"))
        {
            _player.Log.Clear();
            _activeEventLog.Clear();
        }
        ImGui.Separator();

        if (ImGui.BeginChild("CsLog", 0, 0))
        {
            foreach (var line in _player.Log)
                ImGui.Text(line);
            foreach (var line in _activeEventLog)
                ImGui.Text(line);
        }
        ImGui.EndChild();
    }
}
