using Luma.SDK.Plugins;
namespace QuickNotes;

public class QuickNotesPlugin : EditorPlugin
{
    public override string Id => "com.luma.quicknotes";
    public override string Name => "快速笔记";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "开发待办与场景/实体笔记管理插件";

    private QuickNotesPanel? _panel;

    public override void OnLoad()
    {
        base.OnLoad();
        _panel = RegisterPanel<QuickNotesPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("笔记"))
        {
            if (ImGui.MenuItem("打开笔记面板"))
            {
                if (_panel != null)
                    _panel.IsVisible = true;
            }
            ImGui.Separator();
            if (ImGui.MenuItem("添加 TODO"))
            {
                if (_panel != null)
                {
                    _panel.IsVisible = true;
                    _panel.RequestAddTodo();
                }
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "窗口")
        {
            if (ImGui.MenuItem("快速笔记"))
            {
                if (_panel != null)
                    _panel.IsVisible = true;
            }
        }
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("添加场景笔记"))
            {
                if (_panel != null)
                {
                    _panel.IsVisible = true;
                    _panel.RequestAddSceneNote();
                }
            }
        }
    }
}

public enum Priority { High, Medium, Low }

public class TodoItem
{
    public string Title { get; set; } = "";
    public string Description { get; set; } = "";
    public Priority Priority { get; set; } = Priority.Medium;
    public bool Done { get; set; }
}

public class NoteItem
{
    public string Key { get; set; } = "";
    public string Label { get; set; } = "";
    public string Content { get; set; } = "";
}

public class QuickNotesPanel : EditorPanel
{
    public override string Title => "快速笔记";

    private readonly List<TodoItem> _todos = new();
    private readonly List<NoteItem> _sceneNotes = new();
    private readonly List<NoteItem> _entityNotes = new();

    private string _searchText = "";
    private string _newTodoTitle = "";
    private string _newTodoDesc = "";
    private int _newTodoPriority = 1;
    private bool _showAddTodo;
    private bool _showAddSceneNote;
    private string _newSceneNoteContent = "";
    private string _newEntityNoteContent = "";

    public void RequestAddTodo() => _showAddTodo = true;
    public void RequestAddSceneNote() => _showAddSceneNote = true;

    public override void OnGUI()
    {
        DrawStats();
        ImGui.Separator();

        ImGui.Text("搜索:");
        ImGui.SameLine();
        ImGui.InputText("##search", ref _searchText);

        ImGui.Separator();

        if (ImGui.BeginTabBar("NotesTabs"))
        {
            if (ImGui.BeginTabItem("TODO 列表"))
            {
                DrawTodoTab();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("场景笔记"))
            {
                DrawSceneNotesTab();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("实体笔记"))
            {
                DrawEntityNotesTab();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawStats()
    {
        int total = _todos.Count;
        int done = _todos.Count(t => t.Done);
        int pending = total - done;
        ImGui.Text($"TODO 统计: 总计 {total} | 已完成 {done} | 未完成 {pending}");
        if (total > 0)
            ImGui.ProgressBar((float)done / total, $"{done}/{total}");
    }

    private void DrawTodoTab()
    {
        if (ImGui.Button("+ 添加 TODO") || _showAddTodo)
        {
            _showAddTodo = false;
            ImGui.OpenPopup("AddTodoPopup");
        }

        if (ImGui.BeginPopupModal("AddTodoPopup", "添加 TODO"))
        {
            ImGui.Text("标题:");
            ImGui.InputText("##title", ref _newTodoTitle);
            ImGui.Text("描述:");
            ImGui.InputTextMultiline("##desc", ref _newTodoDesc, 0, 60);
            ImGui.Text("优先级:");
            if (ImGui.Selectable("高", _newTodoPriority == 0)) _newTodoPriority = 0;
            if (ImGui.Selectable("中", _newTodoPriority == 1)) _newTodoPriority = 1;
            if (ImGui.Selectable("低", _newTodoPriority == 2)) _newTodoPriority = 2;
            ImGui.Separator();
            if (ImGui.Button("确认") && _newTodoTitle.Length > 0)
            {
                _todos.Add(new TodoItem
                {
                    Title = _newTodoTitle,
                    Description = _newTodoDesc,
                    Priority = (Priority)_newTodoPriority
                });
                _newTodoTitle = "";
                _newTodoDesc = "";
                _newTodoPriority = 1;
                ImGui.CloseCurrentPopup();
            }
            ImGui.SameLine();
            if (ImGui.Button("取消"))
                ImGui.CloseCurrentPopup();
            ImGui.EndPopup();
        }

        ImGui.Separator();

        int removeIdx = -1;
        for (int i = 0; i < _todos.Count; i++)
        {
            var todo = _todos[i];
            if (_searchText.Length > 0 &&
                !todo.Title.Contains(_searchText, StringComparison.OrdinalIgnoreCase) &&
                !todo.Description.Contains(_searchText, StringComparison.OrdinalIgnoreCase))
                continue;

            ImGui.PushID(i);

            var (r, g, b) = todo.Priority switch
            {
                Priority.High => (1.0f, 0.3f, 0.3f),
                Priority.Medium => (1.0f, 0.9f, 0.2f),
                Priority.Low => (0.3f, 0.9f, 0.3f),
                _ => (1.0f, 1.0f, 1.0f)
            };

            ImGui.Checkbox("##done", ref todo.Done);
            ImGui.SameLine();
            string prioTag = todo.Priority switch
            {
                Priority.High => "[高]",
                Priority.Medium => "[中]",
                Priority.Low => "[低]",
                _ => ""
            };
            ImGui.TextColored(r, g, b, 1.0f, $"{prioTag} {todo.Title}");

            if (todo.Description.Length > 0)
            {
                ImGui.TextWrapped($"  {todo.Description}");
            }

            ImGui.SameLine();
            if (ImGui.SmallButton("删除"))
                removeIdx = i;

            ImGui.Separator();
            ImGui.PopID();
        }

        if (removeIdx >= 0)
            _todos.RemoveAt(removeIdx);
    }

    private void DrawSceneNotesTab()
    {
        string sceneName = LumaAPI.GetCurrentSceneName();
        ImGui.Text($"当前场景: {sceneName}");
        ImGui.Separator();

        if (ImGui.Button("+ 添加场景笔记") || _showAddSceneNote)
        {
            _showAddSceneNote = false;
            ImGui.OpenPopup("AddSceneNotePopup");
        }

        if (ImGui.BeginPopupModal("AddSceneNotePopup", "添加场景笔记"))
        {
            ImGui.Text($"场景: {sceneName}");
            ImGui.Text("内容:");
            ImGui.InputTextMultiline("##scenenote", ref _newSceneNoteContent, 0, 80);
            ImGui.Separator();
            if (ImGui.Button("确认") && _newSceneNoteContent.Length > 0)
            {
                _sceneNotes.Add(new NoteItem
                {
                    Key = sceneName,
                    Label = sceneName,
                    Content = _newSceneNoteContent
                });
                _newSceneNoteContent = "";
                ImGui.CloseCurrentPopup();
            }
            ImGui.SameLine();
            if (ImGui.Button("取消"))
                ImGui.CloseCurrentPopup();
            ImGui.EndPopup();
        }

        ImGui.Separator();

        var filtered = _sceneNotes.Where(n =>
            _searchText.Length == 0 ||
            n.Content.Contains(_searchText, StringComparison.OrdinalIgnoreCase) ||
            n.Label.Contains(_searchText, StringComparison.OrdinalIgnoreCase)).ToList();

        int removeIdx = -1;
        for (int i = 0; i < filtered.Count; i++)
        {
            var note = filtered[i];
            ImGui.PushID(i + 10000);
            if (ImGui.CollapsingHeader($"[{note.Label}] 笔记 #{i + 1}"))
            {
                ImGui.TextWrapped(note.Content);
                if (ImGui.SmallButton("删除"))
                    removeIdx = _sceneNotes.IndexOf(note);
            }
            ImGui.PopID();
        }

        if (removeIdx >= 0)
            _sceneNotes.RemoveAt(removeIdx);
    }

    private void DrawEntityNotesTab()
    {
        int selCount = LumaAPI.GetSelectedEntityCount();
        ImGui.Text($"选中实体: {selCount}");
        ImGui.Separator();

        if (selCount > 0)
        {
            string entityName = LumaAPI.GetSelectedEntityName(0);
            var entityGuid = LumaAPI.GetSelectedEntityGuid(0);
            string guidStr = entityGuid.ToString();
            ImGui.Text($"实体: {entityName} ({guidStr})");

            ImGui.Text("笔记内容:");
            ImGui.InputTextMultiline("##entitynote", ref _newEntityNoteContent, 0, 60);
            if (ImGui.Button("添加笔记") && _newEntityNoteContent.Length > 0)
            {
                _entityNotes.Add(new NoteItem
                {
                    Key = guidStr,
                    Label = entityName,
                    Content = _newEntityNoteContent
                });
                _newEntityNoteContent = "";
            }
        }
        else
        {
            ImGui.TextColored(1.0f, 0.8f, 0.2f, 1.0f, "请先在场景中选中一个实体");
        }

        ImGui.Separator();
        ImGui.Text("所有实体笔记:");

        var filtered = _entityNotes.Where(n =>
            _searchText.Length == 0 ||
            n.Content.Contains(_searchText, StringComparison.OrdinalIgnoreCase) ||
            n.Label.Contains(_searchText, StringComparison.OrdinalIgnoreCase)).ToList();

        int removeIdx = -1;
        for (int i = 0; i < filtered.Count; i++)
        {
            var note = filtered[i];
            ImGui.PushID(i + 20000);
            if (ImGui.CollapsingHeader($"[{note.Label}] {note.Key}"))
            {
                ImGui.TextWrapped(note.Content);
                if (ImGui.SmallButton("删除"))
                    removeIdx = _entityNotes.IndexOf(note);
            }
            ImGui.PopID();
        }

        if (removeIdx >= 0)
            _entityNotes.RemoveAt(removeIdx);
    }
}
