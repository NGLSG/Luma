using Luma.SDK.Plugins;
namespace Localization;

public class LocalizationPlugin : EditorPlugin
{
    public override string Id => "com.luma.localization";
    public override string Name => "本地化";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "多语言本地化管理，支持翻译表编辑和运行时语言切换";

    private LocalizationEditorPanel? _panel;

    public override void OnLoad()
    {
        base.OnLoad();
        _panel = RegisterPanel<LocalizationEditorPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("本地化"))
        {
            if (ImGui.MenuItem("打开编辑器"))
            {
                if (_panel != null) _panel.IsVisible = true;
            }
            ImGui.Separator();
            if (ImGui.MenuItem("新建翻译表"))
            {
                var table = new LocalizationTable($"Table_{LocalizationManager.Instance.Tables.Count + 1}");
                table.AddLanguage("zh-CN", "中文");
                table.AddLanguage("en", "English");
                LocalizationManager.Instance.AddTable(table);
                Log.Info($"已创建翻译表: {table.TableName}");
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("本地化"))
            {
                if (_panel != null) _panel.IsVisible = true;
            }
        }
    }
}

public class LocalizationEditorPanel : EditorPanel
{
    public override string Title => "本地化编辑器";

    private string _newLangCode = "";
    private string _newLangName = "";
    private string _newEntryKey = "";
    private string _searchFilter = "";
    private string _newTableName = "";
    private int _selectedTableIndex = 0;
    private int _previewLangIndex = 0;
    private readonly Dictionary<string, string> _editBuffers = new();

    public LocalizationEditorPanel()
    {
        InitPresets();
    }

    private void InitPresets()
    {
        var table = new LocalizationTable("Main");
        table.AddLanguage("zh-CN", "中文");
        table.AddLanguage("en", "English");
        table.AddLanguage("ja", "日本語");
        table.AddLanguage("ko", "한국어");

        table.AddEntry("app.title");
        table.SetTranslation("app.title", "zh-CN", "我的游戏");
        table.SetTranslation("app.title", "en", "My Game");
        table.SetTranslation("app.title", "ja", "マイゲーム");
        table.SetTranslation("app.title", "ko", "내 게임");

        table.AddEntry("menu.start");
        table.SetTranslation("menu.start", "zh-CN", "开始游戏");
        table.SetTranslation("menu.start", "en", "Start Game");
        table.SetTranslation("menu.start", "ja", "ゲーム開始");
        table.SetTranslation("menu.start", "ko", "게임 시작");

        table.AddEntry("menu.settings");
        table.SetTranslation("menu.settings", "zh-CN", "设置");
        table.SetTranslation("menu.settings", "en", "Settings");
        table.SetTranslation("menu.settings", "ja", "設定");
        table.SetTranslation("menu.settings", "ko", "설정");

        table.AddEntry("menu.exit");
        table.SetTranslation("menu.exit", "zh-CN", "退出");
        table.SetTranslation("menu.exit", "en", "Exit");

        LocalizationManager.Instance.AddTable(table);
    }

    public override void OnGUI()
    {
        DrawTableSelector();
        ImGui.Separator();

        if (ImGui.BeginTabBar("LocalizationTabs"))
        {
            if (ImGui.BeginTabItem("翻译表编辑"))
            {
                DrawTranslationEditor();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("翻译覆盖率"))
            {
                DrawCoverageTab();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("预览"))
            {
                DrawPreviewTab();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawTableSelector()
    {
        var tables = LocalizationManager.Instance.Tables;
        ImGui.Text($"翻译表 ({tables.Count})");
        ImGui.SameLine();

        ImGui.InputText("##newtable", ref _newTableName);
        ImGui.SameLine();
        if (ImGui.SmallButton("新建表"))
        {
            if (!string.IsNullOrWhiteSpace(_newTableName))
            {
                var t = new LocalizationTable(_newTableName);
                t.AddLanguage("zh-CN", "中文");
                t.AddLanguage("en", "English");
                LocalizationManager.Instance.AddTable(t);
                _newTableName = "";
            }
        }

        for (int i = 0; i < tables.Count; i++)
        {
            if (ImGui.Selectable($"{tables[i].TableName}##tbl_{i}", i == _selectedTableIndex))
                _selectedTableIndex = i;
        }

        if (_selectedTableIndex >= tables.Count && tables.Count > 0)
            _selectedTableIndex = tables.Count - 1;
    }

    private LocalizationTable? GetCurrentTable()
    {
        var tables = LocalizationManager.Instance.Tables;
        if (tables.Count == 0 || _selectedTableIndex >= tables.Count) return null;
        return tables[_selectedTableIndex];
    }

    private void DrawTranslationEditor()
    {
        var table = GetCurrentTable();
        if (table == null) { ImGui.Text("请先创建翻译表"); return; }

        if (ImGui.CollapsingHeader("语言管理"))
        {
            for (int i = 0; i < table.Languages.Count; i++)
            {
                var lang = table.Languages[i];
                ImGui.PushID(i);
                ImGui.Text($"{lang.DisplayName} ({lang.Code})");
                ImGui.SameLine();
                if (ImGui.SmallButton("移除"))
                {
                    table.RemoveLanguage(lang.Code);
                    ImGui.PopID();
                    return;
                }
                ImGui.PopID();
            }
            ImGui.Separator();
            ImGui.InputText("语言代码", ref _newLangCode);
            ImGui.SameLine();
            ImGui.InputText("显示名称", ref _newLangName);
            ImGui.SameLine();
            if (ImGui.SmallButton("添加语言"))
            {
                if (!string.IsNullOrWhiteSpace(_newLangCode) && !string.IsNullOrWhiteSpace(_newLangName))
                {
                    table.AddLanguage(_newLangCode, _newLangName);
                    Log.Info($"已添加语言: {_newLangName} ({_newLangCode})");
                    _newLangCode = "";
                    _newLangName = "";
                }
            }
        }

        ImGui.Separator();
        ImGui.InputText("搜索 Key", ref _searchFilter);

        ImGui.InputText("##newkey", ref _newEntryKey);
        ImGui.SameLine();
        if (ImGui.Button("添加条目"))
        {
            if (!string.IsNullOrWhiteSpace(_newEntryKey))
            {
                table.AddEntry(_newEntryKey);
                _newEntryKey = "";
            }
        }

        ImGui.Separator();

        int columns = 2 + table.Languages.Count;
        if (ImGui.BeginTable("TranslationTable", columns))
        {
            ImGui.TableSetupColumn("Key");
            foreach (var lang in table.Languages)
                ImGui.TableSetupColumn(lang.DisplayName);
            ImGui.TableSetupColumn("操作");
            ImGui.TableHeadersRow();

            for (int ei = 0; ei < table.Entries.Count; ei++)
            {
                var entry = table.Entries[ei];

                if (!string.IsNullOrEmpty(_searchFilter) &&
                    !entry.Key.Contains(_searchFilter, StringComparison.OrdinalIgnoreCase))
                    continue;

                ImGui.TableNextRow();
                ImGui.PushID(ei);

                ImGui.TableNextColumn();
                ImGui.Text(entry.Key);

                foreach (var lang in table.Languages)
                {
                    ImGui.TableNextColumn();
                    string bufKey = $"{entry.Key}_{lang.Code}";
                    if (!_editBuffers.ContainsKey(bufKey))
                        _editBuffers[bufKey] = entry.Translations.GetValueOrDefault(lang.Code, "");

                    string buf = _editBuffers[bufKey];
                    if (ImGui.InputText($"##{bufKey}", ref buf))
                    {
                        _editBuffers[bufKey] = buf;
                        table.SetTranslation(entry.Key, lang.Code, buf);
                    }
                }

                ImGui.TableNextColumn();
                if (ImGui.SmallButton("删除"))
                {
                    table.RemoveEntry(entry.Key);
                    ImGui.PopID();
                    break;
                }

                ImGui.PopID();
            }

            ImGui.EndTable();
        }
    }

    private void DrawCoverageTab()
    {
        var table = GetCurrentTable();
        if (table == null) { ImGui.Text("请先创建翻译表"); return; }

        ImGui.Text($"翻译表: {table.TableName} ({table.Entries.Count} 条目)");
        ImGui.Separator();

        foreach (var lang in table.Languages)
        {
            var missing = table.FindMissingTranslations(lang.Code);
            int total = table.Entries.Count;
            int translated = total - missing.Count;
            float ratio = total == 0 ? 1f : (float)translated / total;

            ImGui.Text($"{lang.DisplayName} ({lang.Code})");
            ImGui.ProgressBar(ratio, $"{(int)(ratio * 100)}% ({translated}/{total})");

            if (missing.Count > 0)
            {
                ImGui.Indent();
                ImGui.TextColored(1f, 0.3f, 0.3f, 1f, $"缺失 {missing.Count} 条:");
                foreach (var key in missing)
                    ImGui.TextColored(1f, 0.5f, 0.2f, 1f, $"  • {key}");
                ImGui.Unindent();
            }
            ImGui.Spacing();
        }
    }

    private void DrawPreviewTab()
    {
        var table = GetCurrentTable();
        if (table == null) { ImGui.Text("请先创建翻译表"); return; }

        ImGui.Text("选择预览语言:");
        for (int i = 0; i < table.Languages.Count; i++)
        {
            ImGui.SameLine();
            if (ImGui.Selectable($"{table.Languages[i].DisplayName}##prev_{i}", i == _previewLangIndex))
                _previewLangIndex = i;
        }

        if (_previewLangIndex >= table.Languages.Count)
            _previewLangIndex = 0;

        ImGui.Separator();
        ImGui.Text("多语言对比:");

        int cols = 1 + table.Languages.Count;
        if (ImGui.BeginTable("PreviewTable", cols))
        {
            ImGui.TableSetupColumn("Key");
            foreach (var lang in table.Languages)
                ImGui.TableSetupColumn(lang.DisplayName);
            ImGui.TableHeadersRow();

            foreach (var entry in table.Entries)
            {
                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                ImGui.Text(entry.Key);

                for (int li = 0; li < table.Languages.Count; li++)
                {
                    ImGui.TableNextColumn();
                    string langCode = table.Languages[li].Code;
                    string text = table.GetTranslation(entry.Key, langCode);

                    if (li == _previewLangIndex)
                        ImGui.TextColored(0.2f, 0.8f, 1f, 1f, text);
                    else if (text == entry.Key)
                        ImGui.TextDisabled(text);
                    else
                        ImGui.Text(text);
                }
            }

            ImGui.EndTable();
        }
    }
}
