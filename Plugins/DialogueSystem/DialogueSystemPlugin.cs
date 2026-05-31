using Luma.SDK.Plugins;

namespace DialogueSystem;

public class DialogueSystemPlugin : EditorPlugin
{
    public override string Id => "com.luma.dialogue";
    public override string Name => "对话系统";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "可视化对话树编辑器与运行时对话管理。";

    private DialogueEditorPanel? _editorPanel;
    private DialoguePreviewPanel? _previewPanel;

    public override void OnLoad()
    {
        base.OnLoad();
        _editorPanel = RegisterPanel<DialogueEditorPanel>();
        _previewPanel = RegisterPanel<DialoguePreviewPanel>();
        _editorPanel.SharedDatabase = _previewPanel.SharedDatabase = new DialogueDatabase();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("对话"))
        {
            if (ImGui.MenuItem("新建对话树"))
            {
                _editorPanel?.CreateNewTree();
            }
            ImGui.Separator();
            if (ImGui.MenuItem("打开编辑器"))
            {
                if (_editorPanel != null) _editorPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("打开预览器"))
            {
                if (_previewPanel != null) _previewPanel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("对话系统"))
            {
                if (_editorPanel != null) _editorPanel.IsVisible = true;
            }
        }
    }
}

public class DialogueEditorPanel : EditorPanel
{
    public override string Title => "对话编辑器";

    public DialogueDatabase SharedDatabase { get; set; } = new();

    private DialogueTree? _selectedTree;
    private DialogueNode? _selectedNode;
    private int _nextTreeId = 1;
    private int _nextNodeId = 1;

    private string _newVarName = "";
    private string _newVarValue = "";

    public void CreateNewTree()
    {
        var tree = new DialogueTree
        {
            Id = $"tree_{_nextTreeId++}",
            Name = $"新对话树 {SharedDatabase.Trees.Count + 1}"
        };
        SharedDatabase.AddTree(tree);
        _selectedTree = tree;
        _selectedNode = null;
        Log.Info($"[对话系统] 创建对话树: {tree.Name}");
    }

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("DialogueEditorTabs"))
        {
            if (ImGui.BeginTabItem("对话树"))
            {
                DrawTreeEditor();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("变量管理"))
            {
                DrawVariableManager();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawTreeEditor()
    {
        if (ImGui.BeginTable("DialogueLayout", 2))
        {
            ImGui.TableSetupColumn("树列表");
            ImGui.TableSetupColumn("节点编辑");
            ImGui.TableHeadersRow();

            ImGui.TableNextRow();
            ImGui.TableNextColumn();
            DrawTreeList();
            ImGui.TableNextColumn();
            DrawNodeEditor();

            ImGui.EndTable();
        }
    }

    private void DrawTreeList()
    {
        ImGui.Text("对话树列表");
        ImGui.Separator();

        if (ImGui.Button("+ 新建"))
        {
            CreateNewTree();
        }

        ImGui.Separator();

        string? toRemove = null;
        for (int i = 0; i < SharedDatabase.Trees.Count; i++)
        {
            var tree = SharedDatabase.Trees[i];
            ImGui.PushID($"tree_{i}");
            bool selected = _selectedTree == tree;
            if (ImGui.Selectable($"{tree.Name} ({tree.Nodes.Count})", selected))
            {
                _selectedTree = tree;
                _selectedNode = null;
            }
            ImGui.SameLine();
            if (ImGui.SmallButton("X"))
            {
                toRemove = tree.Id;
            }
            ImGui.PopID();
        }

        if (toRemove != null)
        {
            SharedDatabase.Trees.RemoveAll(t => t.Id == toRemove);
            if (_selectedTree?.Id == toRemove)
            {
                _selectedTree = null;
                _selectedNode = null;
            }
        }
    }

    private void DrawNodeEditor()
    {
        if (_selectedTree == null)
        {
            ImGui.TextDisabled("请选择一个对话树");
            return;
        }

        ImGui.Text($"编辑: {_selectedTree.Name}");
        var treeName = _selectedTree.Name;
        if (ImGui.InputText("树名称", ref treeName))
        {
            _selectedTree.Name = treeName;
        }

        ImGui.Separator();

        if (ImGui.Button("+ 添加节点"))
        {
            var node = new DialogueNode
            {
                Id = $"node_{_nextNodeId++}",
                Speaker = "角色",
                Text = "对话内容"
            };
            _selectedTree.AddNode(node);
            _selectedNode = node;
        }

        ImGui.SameLine();
        ImGui.Text($"起始节点: {_selectedTree.StartNodeId}");

        ImGui.Separator();

        if (_selectedTree.Nodes.Count > 0)
        {
            if (ImGui.BeginTable("NodeTable", 4))
            {
                ImGui.TableSetupColumn("ID");
                ImGui.TableSetupColumn("说话者");
                ImGui.TableSetupColumn("内容预览");
                ImGui.TableSetupColumn("选项数");
                ImGui.TableHeadersRow();

                string? nodeToRemove = null;
                foreach (var kvp in _selectedTree.Nodes)
                {
                    var node = kvp.Value;
                    ImGui.TableNextRow();

                    ImGui.TableNextColumn();
                    ImGui.PushID($"nsel_{node.Id}");
                    if (ImGui.Selectable(node.Id, _selectedNode == node))
                    {
                        _selectedNode = node;
                    }
                    ImGui.PopID();

                    ImGui.TableNextColumn();
                    ImGui.Text(node.Speaker);

                    ImGui.TableNextColumn();
                    string preview = node.Text.Length > 20 ? node.Text[..20] + "..." : node.Text;
                    ImGui.TextDisabled(preview);

                    ImGui.TableNextColumn();
                    ImGui.Text($"{node.Choices.Count}");
                }

                ImGui.EndTable();

                if (nodeToRemove != null)
                    _selectedTree.RemoveNode(nodeToRemove);
            }
        }

        ImGui.Separator();
        DrawNodeDetails();
    }

    private void DrawNodeDetails()
    {
        if (_selectedNode == null)
        {
            ImGui.TextDisabled("请选择一个节点");
            return;
        }

        if (ImGui.CollapsingHeader("节点详情"))
        {
            ImGui.Text($"节点 ID: {_selectedNode.Id}");

            var speaker = _selectedNode.Speaker;
            if (ImGui.InputText("说话者", ref speaker))
                _selectedNode.Speaker = speaker;

            var text = _selectedNode.Text;
            ImGui.Text("对话内容:");
            if (ImGui.InputTextMultiline("##NodeText", ref text, 0, 80))
                _selectedNode.Text = text;

            var nextNode = _selectedNode.NextNodeId;
            if (ImGui.InputText("下一节点ID (无选项时)", ref nextNode))
                _selectedNode.NextNodeId = nextNode;

            ImGui.Spacing();
            if (ImGui.Button("删除此节点"))
            {
                _selectedTree?.RemoveNode(_selectedNode.Id);
                _selectedNode = null;
                return;
            }

            if (_selectedNode.Id != _selectedTree?.StartNodeId)
            {
                ImGui.SameLine();
                if (ImGui.Button("设为起始节点"))
                {
                    if (_selectedTree != null)
                        _selectedTree.StartNodeId = _selectedNode.Id;
                }
            }
        }

        if (_selectedNode == null) return;

        if (ImGui.CollapsingHeader("选项编辑"))
        {
            if (ImGui.Button("+ 添加选项"))
            {
                _selectedNode.Choices.Add(new DialogueChoice
                {
                    Text = "选项文本",
                    NextNodeId = ""
                });
            }

            int choiceToRemove = -1;
            for (int i = 0; i < _selectedNode.Choices.Count; i++)
            {
                var choice = _selectedNode.Choices[i];
                ImGui.PushID($"choice_{i}");
                ImGui.Separator();
                ImGui.Text($"选项 {i + 1}");

                var choiceText = choice.Text;
                if (ImGui.InputText("文本", ref choiceText))
                    choice.Text = choiceText;

                var choiceNext = choice.NextNodeId;
                if (ImGui.InputText("目标节点", ref choiceNext))
                    choice.NextNodeId = choiceNext;

                var choiceCond = choice.Condition;
                if (ImGui.InputText("条件", ref choiceCond))
                    choice.Condition = choiceCond;

                if (ImGui.SmallButton("删除"))
                    choiceToRemove = i;

                ImGui.PopID();
            }

            if (choiceToRemove >= 0)
                _selectedNode.Choices.RemoveAt(choiceToRemove);
        }
    }

    private void DrawVariableManager()
    {
        ImGui.Text("对话变量管理");
        ImGui.Separator();

        ImGui.InputText("变量名", ref _newVarName);
        ImGui.SameLine();
        ImGui.InputText("值", ref _newVarValue);
        ImGui.SameLine();
        if (ImGui.Button("添加"))
        {
            if (!string.IsNullOrWhiteSpace(_newVarName))
            {
                SharedDatabase.SetVariable(_newVarName, _newVarValue);
                _newVarName = "";
                _newVarValue = "";
            }
        }

        ImGui.Separator();

        if (SharedDatabase.Variables.Count == 0)
        {
            ImGui.TextDisabled("暂无变量");
            return;
        }

        if (ImGui.BeginTable("VarTable", 3))
        {
            ImGui.TableSetupColumn("变量名");
            ImGui.TableSetupColumn("值");
            ImGui.TableSetupColumn("操作");
            ImGui.TableHeadersRow();

            string? varToRemove = null;
            foreach (var kvp in SharedDatabase.Variables)
            {
                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                ImGui.Text(kvp.Key);

                ImGui.TableNextColumn();
                ImGui.PushID($"var_{kvp.Key}");
                var val = kvp.Value.Value;
                if (ImGui.InputText("##val", ref val))
                    kvp.Value.Value = val;
                ImGui.PopID();

                ImGui.TableNextColumn();
                ImGui.PushID($"vardel_{kvp.Key}");
                if (ImGui.SmallButton("删除"))
                    varToRemove = kvp.Key;
                ImGui.PopID();
            }

            ImGui.EndTable();

            if (varToRemove != null)
                SharedDatabase.Variables.Remove(varToRemove);
        }
    }
}

public class DialoguePreviewPanel : EditorPanel
{
    public override string Title => "对话预览";

    public DialogueDatabase SharedDatabase { get; set; } = new();

    private readonly DialogueRuntime _runtime = new();
    private readonly List<string> _history = new();
    private int _selectedTreeIndex;

    public override void OnCreate()
    {
        _runtime.OnNodeChanged += node =>
        {
            _history.Add($"[{node.Speaker}] {node.Text}");
        };
        _runtime.OnDialogueEnd += () =>
        {
            _history.Add("--- 对话结束 ---");
        };
    }

    public override void OnGUI()
    {
        DrawControls();
        ImGui.Separator();
        DrawDialogueView();
        ImGui.Separator();
        DrawHistory();
    }

    private void DrawControls()
    {
        ImGui.Text("对话预览控制");
        ImGui.Separator();

        if (SharedDatabase.Trees.Count == 0)
        {
            ImGui.TextDisabled("请先在编辑器中创建对话树");
            return;
        }

        var treeNames = SharedDatabase.Trees.Select(t => t.Name).ToArray();
        ImGui.Text($"选择对话树: {(_selectedTreeIndex < treeNames.Length ? treeNames[_selectedTreeIndex] : "无")}");

        for (int i = 0; i < SharedDatabase.Trees.Count; i++)
        {
            ImGui.PushID($"tsel_{i}");
            if (ImGui.Selectable(SharedDatabase.Trees[i].Name, _selectedTreeIndex == i))
                _selectedTreeIndex = i;
            ImGui.PopID();
        }

        ImGui.Separator();

        if (ImGui.Button("开始对话"))
        {
            if (_selectedTreeIndex < SharedDatabase.Trees.Count)
            {
                _history.Clear();
                _runtime.StartDialogue(SharedDatabase.Trees[_selectedTreeIndex]);
            }
        }

        ImGui.SameLine();
        if (ImGui.Button("重新开始"))
        {
            if (_selectedTreeIndex < SharedDatabase.Trees.Count)
            {
                _runtime.EndDialogue();
                _history.Clear();
                _runtime.StartDialogue(SharedDatabase.Trees[_selectedTreeIndex]);
            }
        }

        ImGui.SameLine();
        if (ImGui.Button("停止"))
        {
            _runtime.EndDialogue();
        }
    }

    private void DrawDialogueView()
    {
        if (!_runtime.IsActive)
        {
            ImGui.TextDisabled("对话未运行");
            return;
        }

        var node = _runtime.CurrentNode!;
        ImGui.TextColored(0.3f, 0.7f, 1.0f, 1.0f, $"[{node.Speaker}]");
        ImGui.TextWrapped(node.Text);
        ImGui.Spacing();

        if (node.Choices.Count > 0)
        {
            ImGui.Text("选择:");
            for (int i = 0; i < node.Choices.Count; i++)
            {
                var choice = node.Choices[i];
                bool condMet = DialogueRuntime.EvaluateCondition(choice.Condition, SharedDatabase);

                ImGui.PushID($"choice_{i}");
                if (condMet)
                {
                    if (ImGui.Button($"{i + 1}. {choice.Text}"))
                    {
                        _runtime.SelectChoice(i);
                    }
                }
                else
                {
                    ImGui.TextDisabled($"{i + 1}. {choice.Text} [条件不满足]");
                }
                ImGui.PopID();
            }
        }
        else
        {
            if (ImGui.Button("继续 >>"))
            {
                _runtime.Advance();
            }
        }
    }

    private void DrawHistory()
    {
        if (ImGui.CollapsingHeader("对话历史"))
        {
            if (_history.Count == 0)
            {
                ImGui.TextDisabled("暂无历史记录");
                return;
            }

            for (int i = 0; i < _history.Count; i++)
            {
                ImGui.TextWrapped(_history[i]);
            }
        }
    }
}
