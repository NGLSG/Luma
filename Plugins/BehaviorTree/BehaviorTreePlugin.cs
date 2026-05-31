using Luma.SDK.Plugins;
namespace BehaviorTree;

public class BehaviorTreePlugin : EditorPlugin
{
    public override string Id => "com.luma.behaviortree";
    public override string Name => "行为树";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "AI行为树编辑器与运行时，用于游戏 AI 决策逻辑";

    private BTEditorPanel? _editorPanel;
    private BTDebugPanel? _debugPanel;

    public override void OnLoad()
    {
        base.OnLoad();
        _editorPanel = RegisterPanel<BTEditorPanel>();
        _debugPanel = RegisterPanel<BTDebugPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("行为树"))
        {
            if (ImGui.MenuItem("新建行为树"))
            {
                if (_editorPanel != null)
                {
                    _editorPanel.IsVisible = true;
                    _editorPanel.CreateNewTree();
                }
            }
            if (ImGui.MenuItem("打开编辑器"))
            {
                if (_editorPanel != null)
                    _editorPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("打开调试器"))
            {
                if (_debugPanel != null)
                    _debugPanel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "窗口")
        {
            if (ImGui.MenuItem("行为树编辑器"))
            {
                if (_editorPanel != null)
                    _editorPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("行为树调试器"))
            {
                if (_debugPanel != null)
                    _debugPanel.IsVisible = true;
            }
        }
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("行为树"))
            {
                if (_editorPanel != null)
                    _editorPanel.IsVisible = true;
            }
        }
    }
}

public class BTEditorPanel : EditorPanel
{
    public override string Title => "行为树编辑器";

    private readonly List<BehaviorTreeAsset> _trees = new();
    private int _selectedTreeIndex = -1;
    private BTNode? _selectedNode;
    private string _newTreeName = "NewTree";

    private string _editName = "";
    private float _editFloat = 0;
    private int _editInt = 0;
    private string _editString = "";

    public void CreateNewTree()
    {
        var tree = new BehaviorTreeAsset { Name = $"Tree_{_trees.Count + 1}" };
        tree.RootNode = new SequenceNode { Name = "Root" };
        _trees.Add(tree);
        _selectedTreeIndex = _trees.Count - 1;
        _selectedNode = tree.RootNode;
    }

    public BehaviorTreeAsset? GetSelectedTree()
    {
        if (_selectedTreeIndex >= 0 && _selectedTreeIndex < _trees.Count)
            return _trees[_selectedTreeIndex];
        return null;
    }

    public override void OnGUI()
    {
        if (ImGui.BeginChild("LeftPane", 200, 0))
        {
            DrawTreeList();
        }
        ImGui.EndChild();

        ImGui.SameLine();

        if (ImGui.BeginChild("RightPane", 0, 0))
        {
            DrawRightPane();
        }
        ImGui.EndChild();
    }

    private void DrawTreeList()
    {
        ImGui.Text("行为树列表");
        ImGui.Separator();

        for (int i = 0; i < _trees.Count; i++)
        {
            ImGui.PushID(i);
            if (ImGui.Selectable(_trees[i].Name, _selectedTreeIndex == i))
            {
                _selectedTreeIndex = i;
                _selectedNode = _trees[i].RootNode;
            }
            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.InputText("##newname", ref _newTreeName);
        ImGui.SameLine();
        if (ImGui.Button("+"))
        {
            var tree = new BehaviorTreeAsset { Name = _newTreeName };
            tree.RootNode = new SequenceNode { Name = "Root" };
            _trees.Add(tree);
            _selectedTreeIndex = _trees.Count - 1;
            _newTreeName = $"Tree_{_trees.Count + 1}";
        }

        if (_selectedTreeIndex >= 0 && ImGui.Button("删除"))
        {
            _trees.RemoveAt(_selectedTreeIndex);
            _selectedTreeIndex = Math.Min(_selectedTreeIndex, _trees.Count - 1);
            _selectedNode = null;
        }
    }

    private void DrawRightPane()
    {
        var tree = GetSelectedTree();
        if (tree == null)
        {
            ImGui.Text("请选择或新建行为树");
            return;
        }

        if (ImGui.BeginTabBar("EditorTabs"))
        {
            if (ImGui.BeginTabItem("树结构"))
            {
                DrawTreeStructure(tree);
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("节点属性"))
            {
                DrawNodeProperties();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("添加节点"))
            {
                DrawAddNode();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawTreeStructure(BehaviorTreeAsset tree)
    {
        ImGui.Text($"行为树: {tree.Name}");
        ImGui.Separator();
        if (tree.RootNode != null)
            DrawNodeTree(tree.RootNode);
        else
            ImGui.TextDisabled("空树");
    }

    private void DrawNodeTree(BTNode node)
    {
        ImGui.PushID(node.Id);

        var (r, g, b) = GetNodeColor(node);
        string typeTag = GetNodeTypeTag(node);

        bool hasChildren = node.Children.Count > 0;
        bool open = hasChildren ? ImGui.TreeNode($"{typeTag} {node.Name}") : false;

        if (!hasChildren)
        {
            ImGui.TextColored(r, g, b, 1.0f, $"  {typeTag} {node.Name}");
        }

        if (ImGui.Selectable($"##sel_{node.Id}", _selectedNode == node))
        {
            _selectedNode = node;
            _editName = node.Name;
        }

        if (open && hasChildren)
        {
            foreach (var child in node.Children)
                DrawNodeTree(child);
            ImGui.TreePop();
        }

        ImGui.PopID();
    }

    private void DrawNodeProperties()
    {
        if (_selectedNode == null)
        {
            ImGui.TextDisabled("未选中节点");
            return;
        }

        var node = _selectedNode;
        ImGui.Text($"类型: {GetNodeTypeTag(node)}");
        ImGui.Text($"ID: {node.Id}");
        ImGui.Separator();

        _editName = node.Name;
        if (ImGui.InputText("名称", ref _editName))
            node.Name = _editName;

        ImGui.Separator();

        switch (node)
        {
            case WaitNode wait:
                _editFloat = wait.Duration;
                if (ImGui.SliderFloat("等待时间", ref _editFloat, 0.1f, 30.0f))
                    wait.Duration = _editFloat;
                break;
            case LogNode log:
                _editString = log.Message;
                if (ImGui.InputText("消息", ref _editString))
                    log.Message = _editString;
                break;
            case RepeatNode repeat:
                _editInt = repeat.RepeatCount;
                if (ImGui.SliderInt("重复次数", ref _editInt, 1, 100))
                    repeat.RepeatCount = _editInt;
                break;
            case RetryNode retry:
                _editInt = retry.MaxRetries;
                if (ImGui.SliderInt("最大重试", ref _editInt, 1, 50))
                    retry.MaxRetries = _editInt;
                break;
            case CooldownNode cd:
                _editFloat = cd.CooldownTime;
                if (ImGui.SliderFloat("冷却时间", ref _editFloat, 0.1f, 60.0f))
                    cd.CooldownTime = _editFloat;
                break;
            case CheckBlackboardNode check:
                _editString = check.Key;
                if (ImGui.InputText("Key", ref _editString))
                    check.Key = _editString;
                _editString = check.ExpectedValue;
                if (ImGui.InputText("期望值", ref _editString))
                    check.ExpectedValue = _editString;
                break;
            case SetBlackboardNode set:
                _editString = set.Key;
                if (ImGui.InputText("Key", ref _editString))
                    set.Key = _editString;
                _editString = set.Value;
                if (ImGui.InputText("Value", ref _editString))
                    set.Value = _editString;
                break;
        }

        ImGui.Separator();
        ImGui.Text($"子节点数: {node.Children.Count}");

        if (node.Children.Count > 0 && ImGui.Button("清空子节点"))
            node.Children.Clear();
    }

    private void DrawAddNode()
    {
        if (_selectedNode == null)
        {
            ImGui.TextDisabled("请先在树结构中选中一个父节点");
            return;
        }

        ImGui.Text($"添加到: {_selectedNode.Name}");
        ImGui.Separator();

        if (ImGui.CollapsingHeader("组合节点"))
        {
            if (ImGui.Button("Sequence")) AddChild(new SequenceNode());
            ImGui.SameLine();
            if (ImGui.Button("Selector")) AddChild(new SelectorNode());
            ImGui.SameLine();
            if (ImGui.Button("Parallel")) AddChild(new ParallelNode());
        }

        if (ImGui.CollapsingHeader("装饰节点"))
        {
            if (ImGui.Button("Inverter")) AddChild(new InverterNode());
            ImGui.SameLine();
            if (ImGui.Button("Repeat")) AddChild(new RepeatNode());
            ImGui.SameLine();
            if (ImGui.Button("Retry")) AddChild(new RetryNode());
            if (ImGui.Button("Cooldown")) AddChild(new CooldownNode());
            ImGui.SameLine();
            if (ImGui.Button("Succeeder")) AddChild(new SucceederNode());
        }

        if (ImGui.CollapsingHeader("叶子节点"))
        {
            if (ImGui.Button("Wait")) AddChild(new WaitNode());
            ImGui.SameLine();
            if (ImGui.Button("Log")) AddChild(new LogNode());
            if (ImGui.Button("CheckBB")) AddChild(new CheckBlackboardNode());
            ImGui.SameLine();
            if (ImGui.Button("SetBB")) AddChild(new SetBlackboardNode());
        }
    }

    private void AddChild(BTNode node)
    {
        _selectedNode?.Children.Add(node);
    }

    private static (float r, float g, float b) GetNodeColor(BTNode node) => node switch
    {
        SequenceNode or SelectorNode or ParallelNode => (0.4f, 0.7f, 1.0f),
        InverterNode or RepeatNode or RetryNode or CooldownNode or SucceederNode => (0.9f, 0.7f, 0.3f),
        _ => (0.7f, 0.9f, 0.5f)
    };

    private static string GetNodeTypeTag(BTNode node) => node switch
    {
        SequenceNode => "[SEQ]",
        SelectorNode => "[SEL]",
        ParallelNode => "[PAR]",
        InverterNode => "[INV]",
        RepeatNode => "[RPT]",
        RetryNode => "[RTR]",
        CooldownNode => "[CD]",
        SucceederNode => "[SUC]",
        WaitNode => "[WAIT]",
        LogNode => "[LOG]",
        CheckBlackboardNode => "[CHK]",
        SetBlackboardNode => "[SET]",
        _ => "[???]"
    };
}

public class BTDebugPanel : EditorPanel
{
    public override string Title => "行为树调试";

    private readonly BehaviorTreeRunner _runner = new();
    private bool _autoTick;
    private float _tickInterval = 0.5f;
    private float _tickTimer;
    private BTEditorPanel? _editorPanel;

    public void SetEditorPanel(BTEditorPanel panel) => _editorPanel = panel;

    public override void OnGUI()
    {
        DrawControls();
        ImGui.Separator();

        if (ImGui.BeginTabBar("DebugTabs"))
        {
            if (ImGui.BeginTabItem("节点状态"))
            {
                DrawNodeStatus();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("Blackboard"))
            {
                DrawBlackboard();
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
        ImGui.Text($"状态: {(_runner.IsRunning ? "运行中" : "已停止")}");

        if (!_runner.IsRunning)
        {
            if (ImGui.Button("启动"))
            {
                var tree = _editorPanel?.GetSelectedTree();
                if (tree != null)
                    _runner.Start(tree);
            }
        }
        else
        {
            if (ImGui.Button("单步执行"))
                _runner.Update(_tickInterval);

            ImGui.SameLine();
            ImGui.Checkbox("自动", ref _autoTick);

            if (_autoTick)
            {
                ImGui.SameLine();
                ImGui.SliderFloat("间隔", ref _tickInterval, 0.1f, 5.0f);
            }

            ImGui.SameLine();
            if (ImGui.Button("停止"))
                _runner.Stop();

            ImGui.SameLine();
            if (ImGui.Button("重置"))
            {
                var tree = _runner.Tree;
                _runner.Stop();
                if (tree != null)
                    _runner.Start(tree);
            }
        }
    }

    private void DrawNodeStatus()
    {
        if (_runner.Tree?.RootNode == null)
        {
            ImGui.TextDisabled("无行为树");
            return;
        }
        DrawNodeStatusTree(_runner.Tree.RootNode);
    }

    private void DrawNodeStatusTree(BTNode node)
    {
        ImGui.PushID(node.Id);

        var (r, g, b) = node.LastStatus switch
        {
            NodeStatus.Success => (0.2f, 0.9f, 0.2f),
            NodeStatus.Failure => (0.9f, 0.2f, 0.2f),
            NodeStatus.Running => (1.0f, 0.9f, 0.2f),
            _ => (0.5f, 0.5f, 0.5f)
        };

        string statusStr = node.LastStatus.ToString();
        ImGui.TextColored(r, g, b, 1.0f, $"[{statusStr}] {node.Name}");

        if (node.Children.Count > 0)
        {
            ImGui.Indent();
            foreach (var child in node.Children)
                DrawNodeStatusTree(child);
            ImGui.Unindent();
        }

        ImGui.PopID();
    }

    private void DrawBlackboard()
    {
        ImGui.Text("Blackboard");
        ImGui.Separator();

        var bb = _runner.Context.Blackboard;
        if (bb.Count == 0)
        {
            ImGui.TextDisabled("空");
            return;
        }

        if (ImGui.BeginTable("BBTable", 2))
        {
            ImGui.TableSetupColumn("Key");
            ImGui.TableSetupColumn("Value");
            ImGui.TableHeadersRow();

            foreach (var kv in bb)
            {
                ImGui.TableNextRow();
                ImGui.TableNextColumn();
                ImGui.Text(kv.Key);
                ImGui.TableNextColumn();
                ImGui.Text(kv.Value?.ToString() ?? "null");
            }
            ImGui.EndTable();
        }
    }

    private void DrawLog()
    {
        ImGui.Text($"日志 ({_runner.ExecutionLog.Count} 条)");
        ImGui.Separator();

        if (ImGui.Button("清空日志"))
            _runner.ExecutionLog.Clear();

        ImGui.Separator();

        if (ImGui.BeginChild("LogScroll", 0, 0))
        {
            foreach (var line in _runner.ExecutionLog)
                ImGui.Text(line);
        }
        ImGui.EndChild();
    }
}
