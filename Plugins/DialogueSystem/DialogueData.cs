namespace DialogueSystem;

public class DialogueChoice
{
    public string Text { get; set; } = "";
    public string NextNodeId { get; set; } = "";
    public string Condition { get; set; } = "";
}

public class DialogueNode
{
    public string Id { get; set; } = "";
    public string Speaker { get; set; } = "";
    public string Text { get; set; } = "";
    public List<DialogueChoice> Choices { get; set; } = new();
    public string NextNodeId { get; set; } = "";
}

public class DialogueTree
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public string StartNodeId { get; set; } = "";
    public Dictionary<string, DialogueNode> Nodes { get; set; } = new();

    public void AddNode(DialogueNode node)
    {
        Nodes[node.Id] = node;
        if (Nodes.Count == 1)
            StartNodeId = node.Id;
    }

    public void RemoveNode(string nodeId)
    {
        Nodes.Remove(nodeId);
        if (StartNodeId == nodeId && Nodes.Count > 0)
            StartNodeId = Nodes.Keys.First();
        else if (Nodes.Count == 0)
            StartNodeId = "";
    }

    public DialogueNode? GetNode(string nodeId)
    {
        return Nodes.TryGetValue(nodeId, out var node) ? node : null;
    }
}

public class DialogueVariable
{
    public string Name { get; set; } = "";
    public string Value { get; set; } = "";
}

public class DialogueDatabase
{
    public List<DialogueTree> Trees { get; set; } = new();
    public Dictionary<string, DialogueVariable> Variables { get; set; } = new();

    public void AddTree(DialogueTree tree)
    {
        Trees.Add(tree);
    }

    public DialogueTree? GetTree(string id)
    {
        return Trees.FirstOrDefault(t => t.Id == id);
    }

    public void SetVariable(string name, string value)
    {
        if (Variables.TryGetValue(name, out var v))
            v.Value = value;
        else
            Variables[name] = new DialogueVariable { Name = name, Value = value };
    }

    public string GetVariable(string name)
    {
        return Variables.TryGetValue(name, out var v) ? v.Value : "";
    }
}
