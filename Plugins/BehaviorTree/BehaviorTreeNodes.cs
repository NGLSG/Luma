namespace BehaviorTree;

public enum NodeStatus { Success, Failure, Running }

public class BTContext
{
    public Dictionary<string, object> Blackboard { get; } = new();
    public float DeltaTime { get; set; }
}

public abstract class BTNode
{
    public string Id { get; set; } = Guid.NewGuid().ToString("N")[..8];
    public string Name { get; set; } = "";
    public List<BTNode> Children { get; } = new();
    public NodeStatus LastStatus { get; protected set; } = NodeStatus.Failure;

    public abstract NodeStatus Tick(BTContext context);
}

// --- Composite Nodes ---

public class SequenceNode : BTNode
{
    public SequenceNode() { Name = "Sequence"; }

    public override NodeStatus Tick(BTContext context)
    {
        foreach (var child in Children)
        {
            var status = child.Tick(context);
            child.LastStatus = status;
            if (status != NodeStatus.Success)
                return LastStatus = status;
        }
        return LastStatus = NodeStatus.Success;
    }
}

public class SelectorNode : BTNode
{
    public SelectorNode() { Name = "Selector"; }

    public override NodeStatus Tick(BTContext context)
    {
        foreach (var child in Children)
        {
            var status = child.Tick(context);
            child.LastStatus = status;
            if (status != NodeStatus.Failure)
                return LastStatus = status;
        }
        return LastStatus = NodeStatus.Failure;
    }
}

public class ParallelNode : BTNode
{
    public ParallelNode() { Name = "Parallel"; }

    public override NodeStatus Tick(BTContext context)
    {
        int successCount = 0;
        int failCount = 0;
        foreach (var child in Children)
        {
            var status = child.Tick(context);
            child.LastStatus = status;
            if (status == NodeStatus.Success) successCount++;
            else if (status == NodeStatus.Failure) failCount++;
        }
        if (failCount > 0) return LastStatus = NodeStatus.Failure;
        if (successCount == Children.Count) return LastStatus = NodeStatus.Success;
        return LastStatus = NodeStatus.Running;
    }
}

// --- Decorator Nodes ---

public class InverterNode : BTNode
{
    public InverterNode() { Name = "Inverter"; }

    public override NodeStatus Tick(BTContext context)
    {
        if (Children.Count == 0) return LastStatus = NodeStatus.Failure;
        var status = Children[0].Tick(context);
        Children[0].LastStatus = status;
        return LastStatus = status switch
        {
            NodeStatus.Success => NodeStatus.Failure,
            NodeStatus.Failure => NodeStatus.Success,
            _ => NodeStatus.Running
        };
    }
}

public class RepeatNode : BTNode
{
    public int RepeatCount { get; set; } = 3;
    private int _current;

    public RepeatNode() { Name = "Repeat"; }

    public override NodeStatus Tick(BTContext context)
    {
        if (Children.Count == 0) return LastStatus = NodeStatus.Failure;
        while (_current < RepeatCount)
        {
            var status = Children[0].Tick(context);
            Children[0].LastStatus = status;
            if (status == NodeStatus.Running) return LastStatus = NodeStatus.Running;
            if (status == NodeStatus.Failure) { _current = 0; return LastStatus = NodeStatus.Failure; }
            _current++;
        }
        _current = 0;
        return LastStatus = NodeStatus.Success;
    }
}

public class RetryNode : BTNode
{
    public int MaxRetries { get; set; } = 3;
    private int _attempts;

    public RetryNode() { Name = "Retry"; }

    public override NodeStatus Tick(BTContext context)
    {
        if (Children.Count == 0) return LastStatus = NodeStatus.Failure;
        while (_attempts < MaxRetries)
        {
            var status = Children[0].Tick(context);
            Children[0].LastStatus = status;
            if (status == NodeStatus.Running) return LastStatus = NodeStatus.Running;
            if (status == NodeStatus.Success) { _attempts = 0; return LastStatus = NodeStatus.Success; }
            _attempts++;
        }
        _attempts = 0;
        return LastStatus = NodeStatus.Failure;
    }
}

public class CooldownNode : BTNode
{
    public float CooldownTime { get; set; } = 2.0f;
    private float _timer;

    public CooldownNode() { Name = "Cooldown"; }

    public override NodeStatus Tick(BTContext context)
    {
        if (_timer > 0)
        {
            _timer -= context.DeltaTime;
            return LastStatus = NodeStatus.Failure;
        }
        if (Children.Count == 0) return LastStatus = NodeStatus.Failure;
        var status = Children[0].Tick(context);
        Children[0].LastStatus = status;
        if (status != NodeStatus.Running)
            _timer = CooldownTime;
        return LastStatus = status;
    }
}

public class SucceederNode : BTNode
{
    public SucceederNode() { Name = "Succeeder"; }

    public override NodeStatus Tick(BTContext context)
    {
        if (Children.Count > 0)
        {
            Children[0].Tick(context);
        }
        return LastStatus = NodeStatus.Success;
    }
}

// --- Leaf Nodes ---

public class WaitNode : BTNode
{
    public float Duration { get; set; } = 1.0f;
    private float _elapsed;

    public WaitNode() { Name = "Wait"; }

    public override NodeStatus Tick(BTContext context)
    {
        _elapsed += context.DeltaTime;
        if (_elapsed >= Duration)
        {
            _elapsed = 0;
            return LastStatus = NodeStatus.Success;
        }
        return LastStatus = NodeStatus.Running;
    }
}

public class LogNode : BTNode
{
    public string Message { get; set; } = "Log";

    public LogNode() { Name = "Log"; }

    public override NodeStatus Tick(BTContext context)
    {
        return LastStatus = NodeStatus.Success;
    }
}

public class CheckBlackboardNode : BTNode
{
    public string Key { get; set; } = "";
    public string ExpectedValue { get; set; } = "";
    public bool CheckExistence { get; set; } = true;

    public CheckBlackboardNode() { Name = "CheckBB"; }

    public override NodeStatus Tick(BTContext context)
    {
        if (!context.Blackboard.ContainsKey(Key))
            return LastStatus = NodeStatus.Failure;
        if (CheckExistence)
            return LastStatus = NodeStatus.Success;
        var val = context.Blackboard[Key]?.ToString() ?? "";
        return LastStatus = val == ExpectedValue ? NodeStatus.Success : NodeStatus.Failure;
    }
}

public class SetBlackboardNode : BTNode
{
    public string Key { get; set; } = "";
    public string Value { get; set; } = "";

    public SetBlackboardNode() { Name = "SetBB"; }

    public override NodeStatus Tick(BTContext context)
    {
        context.Blackboard[Key] = Value;
        return LastStatus = NodeStatus.Success;
    }
}
