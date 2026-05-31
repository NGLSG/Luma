using Luma.SDK.Plugins;
namespace InputMapper;

public class InputMapperPlugin : EditorPlugin
{
    public override string Id => "com.luma.inputmapper";
    public override string Name => "输入映射";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "可重绑定的输入动作映射系统";

    private InputMapperPanel? _panel;

    public override void OnLoad()
    {
        base.OnLoad();
        _panel = RegisterPanel<InputMapperPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("输入"))
        {
            if (ImGui.MenuItem("打开映射编辑器"))
            {
                if (_panel != null) _panel.IsVisible = true;
            }
            ImGui.Separator();
            if (ImGui.MenuItem("重置默认"))
            {
                InputMapperManager.Instance.ActionMap.ResetToDefault();
                Log.Info("输入映射已重置为默认");
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("输入映射"))
            {
                if (_panel != null) _panel.IsVisible = true;
            }
        }
    }
}

public class InputMapperPanel : EditorPanel
{
    public override string Title => "输入映射编辑器";

    private string _rebindingActionId = "";
    private bool _rebindingIsPrimary = true;
    private string _conflictWarning = "";

    private string _newProfileName = "";
    private string _newActionId = "";
    private string _newActionName = "";
    private string _newActionCategory = "";
    private string _newActionKey = "";
    private string _newActionDesc = "";
    private int _newActionSource = 0;

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("InputMapperTabs"))
        {
            if (ImGui.BeginTabItem("动作映射"))
            {
                DrawActionMapping();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("配置文件管理"))
            {
                DrawProfileManagement();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("新建动作"))
            {
                DrawNewAction();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawActionMapping()
    {
        var map = InputMapperManager.Instance.ActionMap;
        if (map.ActiveProfile == null) { ImGui.Text("无活动配置文件"); return; }

        ImGui.Text($"当前配置: {map.ActiveProfile.Name}");
        ImGui.SameLine();
        if (ImGui.SmallButton("重置全部"))
        {
            map.ResetToDefault();
            Log.Info("已重置所有绑定");
        }

        if (!string.IsNullOrEmpty(_conflictWarning))
        {
            ImGui.TextColored(1f, 0.3f, 0.3f, 1f, _conflictWarning);
        }

        ImGui.Separator();

        var categories = map.ActiveProfile.Actions
            .Select(a => a.Category).Distinct().ToList();

        foreach (var category in categories)
        {
            if (ImGui.CollapsingHeader(category))
            {
                if (ImGui.BeginTable($"Actions_{category}", 5))
                {
                    ImGui.TableSetupColumn("名称");
                    ImGui.TableSetupColumn("说明");
                    ImGui.TableSetupColumn("主绑定");
                    ImGui.TableSetupColumn("副绑定");
                    ImGui.TableSetupColumn("操作");
                    ImGui.TableHeadersRow();

                    var actions = map.ActiveProfile.Actions
                        .Where(a => a.Category == category).ToList();

                    for (int i = 0; i < actions.Count; i++)
                    {
                        var action = actions[i];
                        ImGui.TableNextRow();
                        ImGui.PushID(action.Id);

                        ImGui.TableNextColumn();
                        ImGui.Text(action.Name);

                        ImGui.TableNextColumn();
                        ImGui.TextDisabled(action.Description);

                        ImGui.TableNextColumn();
                        string primaryLabel = _rebindingActionId == action.Id && _rebindingIsPrimary
                            ? "[ 等待输入... ]"
                            : action.PrimaryBinding.DisplayString();
                        if (ImGui.Button($"{primaryLabel}##pri"))
                        {
                            _rebindingActionId = action.Id;
                            _rebindingIsPrimary = true;
                            _conflictWarning = "";
                            ImGui.OpenPopup("RebindPopup");
                        }

                        ImGui.TableNextColumn();
                        string secLabel = action.SecondaryBinding?.DisplayString() ?? "---";
                        if (_rebindingActionId == action.Id && !_rebindingIsPrimary)
                            secLabel = "[ 等待输入... ]";
                        if (ImGui.Button($"{secLabel}##sec"))
                        {
                            _rebindingActionId = action.Id;
                            _rebindingIsPrimary = false;
                            _conflictWarning = "";
                            ImGui.OpenPopup("RebindPopup");
                        }

                        ImGui.TableNextColumn();
                        if (ImGui.SmallButton("重置"))
                        {
                            var defaultProfile = map.Profiles.FirstOrDefault(p => p.IsDefault);
                            var defaultAction = defaultProfile?.Actions.FirstOrDefault(a => a.Id == action.Id);
                            if (defaultAction != null)
                            {
                                action.PrimaryBinding = defaultAction.PrimaryBinding.Clone();
                                action.SecondaryBinding = defaultAction.SecondaryBinding?.Clone();
                                Log.Info($"已重置 {action.Name} 的绑定");
                            }
                        }

                        if (ImGui.BeginPopupModal("RebindPopup"))
                        {
                            ImGui.Text($"为 [{action.Name}] 设置新绑定");
                            ImGui.Text(_rebindingIsPrimary ? "主绑定" : "副绑定");
                            ImGui.Separator();
                            ImGui.Text("请选择按键:");

                            string[] keys = { "W", "A", "S", "D", "E", "F", "Q", "R", "I", "M",
                                "Space", "Escape", "Tab", "Enter", "Shift", "Ctrl",
                                "1", "2", "3", "4", "5" };
                            foreach (var key in keys)
                            {
                                if (ImGui.Selectable(key))
                                {
                                    var newBinding = new InputBinding(InputSource.Keyboard, key);
                                    var conflict = map.HasConflict(newBinding, _rebindingActionId);
                                    if (conflict != null)
                                    {
                                        _conflictWarning = $"冲突: {key} 已绑定到 [{conflict}]";
                                    }
                                    else
                                    {
                                        map.RebindAction(_rebindingActionId, newBinding, _rebindingIsPrimary);
                                        _conflictWarning = "";
                                        Log.Info($"已将 {action.Name} 绑定到 {key}");
                                    }
                                    _rebindingActionId = "";
                                    ImGui.CloseCurrentPopup();
                                }
                            }
                            ImGui.Separator();
                            string[] mouseKeys = { "LeftMouse", "RightMouse", "MiddleMouse" };
                            foreach (var mk in mouseKeys)
                            {
                                if (ImGui.Selectable(mk))
                                {
                                    var newBinding = new InputBinding(InputSource.Mouse, mk);
                                    var conflict = map.HasConflict(newBinding, _rebindingActionId);
                                    if (conflict != null)
                                    {
                                        _conflictWarning = $"冲突: {mk} 已绑定到 [{conflict}]";
                                    }
                                    else
                                    {
                                        map.RebindAction(_rebindingActionId, newBinding, _rebindingIsPrimary);
                                        _conflictWarning = "";
                                        Log.Info($"已将 {action.Name} 绑定到 {mk}");
                                    }
                                    _rebindingActionId = "";
                                    ImGui.CloseCurrentPopup();
                                }
                            }

                            ImGui.Separator();
                            if (ImGui.Button("取消"))
                            {
                                _rebindingActionId = "";
                                ImGui.CloseCurrentPopup();
                            }
                            ImGui.EndPopup();
                        }

                        ImGui.PopID();
                    }

                    ImGui.EndTable();
                }
            }
        }
    }

    private void DrawProfileManagement()
    {
        var map = InputMapperManager.Instance.ActionMap;

        ImGui.Text("配置文件列表:");
        ImGui.Separator();

        for (int i = 0; i < map.Profiles.Count; i++)
        {
            var profile = map.Profiles[i];
            ImGui.PushID(i);

            bool isActive = map.ActiveProfile == profile;
            string label = isActive ? $"▶ {profile.Name}" : $"  {profile.Name}";
            if (profile.IsDefault) label += " (默认)";

            if (ImGui.Selectable($"{label}##prof_{i}", isActive))
                map.SetActiveProfile(profile.Name);

            if (!profile.IsDefault)
            {
                ImGui.SameLine();
                if (ImGui.SmallButton("删除"))
                {
                    map.RemoveProfile(profile.Name);
                    ImGui.PopID();
                    return;
                }
            }

            ImGui.SameLine();
            if (ImGui.SmallButton("复制"))
            {
                var clone = profile.Clone($"{profile.Name}_副本");
                map.Profiles.Add(clone);
                Log.Info($"已复制配置文件: {clone.Name}");
            }

            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.InputText("新配置文件名", ref _newProfileName);
        ImGui.SameLine();
        if (ImGui.Button("新建"))
        {
            if (!string.IsNullOrWhiteSpace(_newProfileName))
            {
                map.AddProfile(_newProfileName);
                var defaultProfile = map.Profiles.FirstOrDefault(p => p.IsDefault);
                var newProfile = map.Profiles.FirstOrDefault(p => p.Name == _newProfileName);
                if (defaultProfile != null && newProfile != null)
                {
                    foreach (var action in defaultProfile.Actions)
                        newProfile.Actions.Add(action.Clone());
                }
                Log.Info($"已创建配置文件: {_newProfileName}");
                _newProfileName = "";
            }
        }
    }

    private void DrawNewAction()
    {
        var map = InputMapperManager.Instance.ActionMap;
        if (map.ActiveProfile == null) { ImGui.Text("无活动配置文件"); return; }

        ImGui.Text("添加新输入动作");
        ImGui.Separator();

        ImGui.InputText("动作 ID", ref _newActionId);
        ImGui.InputText("动作名称", ref _newActionName);
        ImGui.InputText("分类", ref _newActionCategory);
        ImGui.InputText("说明", ref _newActionDesc);
        ImGui.InputText("按键名", ref _newActionKey);

        ImGui.Text("输入源:");
        if (ImGui.Selectable("键盘##src", _newActionSource == 0))
            _newActionSource = 0;
        ImGui.SameLine();
        if (ImGui.Selectable("鼠标##src", _newActionSource == 1))
            _newActionSource = 1;

        ImGui.Separator();
        if (ImGui.Button("添加动作"))
        {
            if (!string.IsNullOrWhiteSpace(_newActionId) &&
                !string.IsNullOrWhiteSpace(_newActionName) &&
                !string.IsNullOrWhiteSpace(_newActionKey))
            {
                var source = _newActionSource == 0 ? InputSource.Keyboard : InputSource.Mouse;
                var binding = new InputBinding(source, _newActionKey);
                var action = new InputAction(_newActionId, _newActionName,
                    string.IsNullOrWhiteSpace(_newActionCategory) ? "自定义" : _newActionCategory,
                    binding, _newActionDesc);

                map.ActiveProfile.Actions.Add(action);
                Log.Info($"已添加动作: {_newActionName} ({_newActionKey})");

                _newActionId = "";
                _newActionName = "";
                _newActionCategory = "";
                _newActionKey = "";
                _newActionDesc = "";
            }
        }
    }
}
