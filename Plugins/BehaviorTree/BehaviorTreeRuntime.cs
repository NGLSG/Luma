namespace BehaviorTree;

public class BehaviorTreeAsset
{
    public string Id { get; set; } = Guid.NewGuid().ToString("N")[..8];
    public string Name { get; set; } = "New Tree";
    public BTNode? RootNode { get; set; }

    public NodeStatus Tick(BTContext context)
    {
        if (RootNode == null) return NodeStatus.Failure;
        var status = RootNode.Tick(context);
        RootNode.LastStatus = status;
        return status;
    }
}

public class BehaviorTreeRunner
{
    public BehaviorTreeAsset? Tree { get; private set; }
    public BTContext Context { get; } = new();
    public bool IsRunning { get; private set; }
    public List<string> ExecutionLog { get; } = new();

    public void Start(BehaviorTreeAsset tree)
    {
        Tree = tree;
        IsRunning = true;
        Context.Blackboard.Clear();
        ExecutionLog.Clear();
        ExecutionLog.Add($"[Start] Tree: {tree.Name}");
    }

    public NodeStatus Update(float deltaTime)
    {
        if (!IsRunning || Tree == null)
            return NodeStatus.Failure;

        Context.DeltaTime = deltaTime;
        var status = Tree.Tick(Context);
        ExecutionLog.Add($"[Tick] dt={deltaTime:F3} -> {status}");
        return status;
    }

    public void Stop()
    {
        IsRunning = false;
        ExecutionLog.Add("[Stop]");
    }

    public void SetBlackboard(string key, object value)
    {
        Context.Blackboard[key] = value;
    }

    public object? GetBlackboard(string key)
    {
        return Context.Blackboard.TryGetValue(key, out var val) ? val : null;
    }
}
