using Luma.SDK.Plugins;
namespace SaveSystem;

public class SaveSystemPlugin : EditorPlugin
{
    public override string Id => "com.luma.savesystem";
    public override string Name => "存档系统";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "游戏存档管理，支持多槽位、自动存档、存档截图";

    private SaveManagerPanel? _panel;

    public override void OnLoad()
    {
        base.OnLoad();
        _panel = RegisterPanel<SaveManagerPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("存档"))
        {
            if (ImGui.MenuItem("新建槽位"))
            {
                SaveManager.Instance.CreateSlot($"存档 {SaveManager.Instance.GetAllSlots().Count + 1}");
                Log.Info("已创建新存档槽位");
            }
            if (ImGui.MenuItem("自动存档"))
            {
                SaveManager.Instance.AutoSave();
                Log.Info("自动存档完成");
            }
            ImGui.Separator();
            if (ImGui.MenuItem("打开管理器"))
            {
                if (_panel != null)
                    _panel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("存档管理"))
            {
                if (_panel != null)
                    _panel.IsVisible = true;
            }
        }
    }
}

public class SaveManagerPanel : EditorPanel
{
    public override string Title => "存档管理";

    private string _newSlotName = "";
    private string _selectedSlotId = "";
    private string _newKey = "";
    private string _newValue = "";
    private string _importJson = "";

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("SaveTabs"))
        {
            if (ImGui.BeginTabItem("存档槽位"))
            {
                DrawSlotList();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("槽位详情"))
            {
                DrawSlotDetail();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("统计"))
            {
                DrawStatistics();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawSlotList()
    {
        var mgr = SaveManager.Instance;
        var slots = mgr.GetAllSlots();

        ImGui.Text("新建存档:");
        ImGui.SameLine();
        ImGui.InputText("##NewSlotName", ref _newSlotName);
        ImGui.SameLine();
        if (ImGui.Button("创建"))
        {
            if (!string.IsNullOrWhiteSpace(_newSlotName))
            {
                var slot = mgr.CreateSlot(_newSlotName);
                _selectedSlotId = slot.Id;
                _newSlotName = "";
                Log.Info($"创建存档槽位: {slot.Name}");
            }
        }

        ImGui.Separator();

        if (ImGui.Button("快速存档"))
        {
            if (!string.IsNullOrEmpty(_selectedSlotId))
            {
                mgr.SaveToSlot(_selectedSlotId);
                Log.Info("快速存档完成");
            }
        }
        ImGui.SameLine();
        if (ImGui.Button("自动存档"))
        {
            mgr.AutoSave();
            Log.Info("自动存档完成");
        }

        ImGui.Separator();

        if (slots.Count == 0)
        {
            ImGui.TextDisabled("暂无存档槽位");
            return;
        }

        if (ImGui.BeginTable("SlotsTable", 6))
        {
            ImGui.TableSetupColumn("名称");
            ImGui.TableSetupColumn("场景");
            ImGui.TableSetupColumn("创建时间");
            ImGui.TableSetupColumn("修改时间");
            ImGui.TableSetupColumn("游戏时间");
            ImGui.TableSetupColumn("操作");
            ImGui.TableHeadersRow();

            string? slotToDelete = null;

            for (int i = 0; i < slots.Count; i++)
            {
                var slot = slots[i];
                ImGui.PushID(i);

                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                bool isSelected = slot.Id == _selectedSlotId;
                if (ImGui.Selectable(slot.Name, isSelected))
                    _selectedSlotId = slot.Id;

                ImGui.TableNextColumn();
                ImGui.Text(string.IsNullOrEmpty(slot.SceneName) ? "-" : slot.SceneName);

                ImGui.TableNextColumn();
                ImGui.TextDisabled(slot.CreatedAt.ToString("MM/dd HH:mm"));

                ImGui.TableNextColumn();
                ImGui.TextDisabled(slot.ModifiedAt.ToString("MM/dd HH:mm"));

                ImGui.TableNextColumn();
                ImGui.Text(slot.PlayTime.ToString(@"hh\:mm\:ss"));

                ImGui.TableNextColumn();
                if (ImGui.SmallButton("读取"))
                {
                    _selectedSlotId = slot.Id;
                    Log.Info($"读取存档: {slot.Name}");
                }
                ImGui.SameLine();
                if (ImGui.SmallButton("删除"))
                {
                    slotToDelete = slot.Id;
                }
                ImGui.SameLine();
                if (ImGui.SmallButton("导出"))
                {
                    var json = mgr.ExportSave(slot.Id);
                    Log.Info($"导出存档 JSON:\n{json}");
                }

                ImGui.PopID();
            }

            ImGui.EndTable();

            if (slotToDelete != null)
            {
                mgr.DeleteSlot(slotToDelete);
                if (_selectedSlotId == slotToDelete)
                    _selectedSlotId = "";
                Log.Info("已删除存档槽位");
            }
        }

        ImGui.Separator();
        if (ImGui.CollapsingHeader("导入存档"))
        {
            ImGui.InputText("JSON##Import", ref _importJson);
            ImGui.SameLine();
            if (ImGui.Button("导入"))
            {
                if (!string.IsNullOrWhiteSpace(_importJson))
                {
                    var imported = mgr.ImportSave(_importJson);
                    if (imported != null)
                    {
                        Log.Info($"成功导入存档: {imported.Name}");
                        _importJson = "";
                    }
                    else
                    {
                        Log.Info("导入失败: JSON格式无效");
                    }
                }
            }
        }
    }

    private void DrawSlotDetail()
    {
        var mgr = SaveManager.Instance;
        var slot = mgr.LoadFromSlot(_selectedSlotId);

        if (slot == null)
        {
            ImGui.TextDisabled("请在「存档槽位」中选择一个槽位");
            return;
        }

        ImGui.Text($"槽位名称: {slot.Name}");
        ImGui.Text($"槽位 ID: {slot.Id}");
        ImGui.Text($"场景: {(string.IsNullOrEmpty(slot.SceneName) ? "-" : slot.SceneName)}");
        ImGui.Text($"创建时间: {slot.CreatedAt:yyyy-MM-dd HH:mm:ss}");
        ImGui.Text($"修改时间: {slot.ModifiedAt:yyyy-MM-dd HH:mm:ss}");
        ImGui.Text($"游戏时间: {slot.PlayTime:hh\\:mm\\:ss}");

        bool isAutoSave = mgr.CurrentProfile.AutoSaveSlotId == slot.Id;
        ImGui.Separator();
        if (isAutoSave)
            ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "✓ 自动存档槽位");
        else if (ImGui.Button("设为自动存档槽位"))
            mgr.CurrentProfile.AutoSaveSlotId = slot.Id;

        ImGui.Separator();
        ImGui.Text("自定义数据");
        ImGui.Separator();

        if (slot.CustomData.Count > 0)
        {
            if (ImGui.BeginTable("CustomDataTable", 3))
            {
                ImGui.TableSetupColumn("键");
                ImGui.TableSetupColumn("值");
                ImGui.TableSetupColumn("操作");
                ImGui.TableHeadersRow();

                string? keyToRemove = null;
                int idx = 0;
                foreach (var kv in slot.CustomData)
                {
                    ImGui.PushID(idx++);
                    ImGui.TableNextRow();
                    ImGui.TableNextColumn();
                    ImGui.Text(kv.Key);
                    ImGui.TableNextColumn();
                    ImGui.Text(kv.Value);
                    ImGui.TableNextColumn();
                    if (ImGui.SmallButton("删除"))
                        keyToRemove = kv.Key;
                    ImGui.PopID();
                }

                ImGui.EndTable();

                if (keyToRemove != null)
                    slot.CustomData.Remove(keyToRemove);
            }
        }
        else
        {
            ImGui.TextDisabled("暂无自定义数据");
        }

        ImGui.Separator();
        ImGui.Text("添加数据:");
        ImGui.InputText("键##AddKey", ref _newKey);
        ImGui.SameLine();
        ImGui.InputText("值##AddValue", ref _newValue);
        ImGui.SameLine();
        if (ImGui.Button("添加"))
        {
            if (!string.IsNullOrWhiteSpace(_newKey))
            {
                mgr.SetCustomData(slot.Id, _newKey, _newValue);
                _newKey = "";
                _newValue = "";
            }
        }
    }

    private void DrawStatistics()
    {
        var mgr = SaveManager.Instance;
        var slots = mgr.GetAllSlots();
        var profile = mgr.CurrentProfile;

        ImGui.Text("存档统计");
        ImGui.Separator();

        ImGui.Text($"总槽位数上限: {profile.MaxSlots}");
        ImGui.Text($"已使用: {slots.Count}");
        ImGui.Text($"剩余: {profile.MaxSlots - slots.Count}");

        float usage = profile.MaxSlots > 0 ? (float)slots.Count / profile.MaxSlots : 0;
        ImGui.ProgressBar(usage, $"{slots.Count}/{profile.MaxSlots}");

        ImGui.Separator();

        if (slots.Count > 0)
        {
            var latest = slots.OrderByDescending(s => s.ModifiedAt).First();
            ImGui.Text($"最近存档: {latest.Name}");
            ImGui.Text($"存档时间: {latest.ModifiedAt:yyyy-MM-dd HH:mm:ss}");

            ImGui.Separator();

            var totalPlayTime = TimeSpan.Zero;
            foreach (var s in slots)
                totalPlayTime += s.PlayTime;
            ImGui.Text($"总游戏时间: {totalPlayTime:hh\\:mm\\:ss}");
        }
        else
        {
            ImGui.TextDisabled("暂无存档数据");
        }
    }
}
