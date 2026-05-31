namespace DialogueSystem;

public class DialogueRuntime
{
    private DialogueTree? _tree;
    private DialogueNode? _currentNode;

    public DialogueNode? CurrentNode => _currentNode;
    public bool IsActive => _tree != null && _currentNode != null;

    public event Action<DialogueNode>? OnNodeChanged;
    public event Action? OnDialogueEnd;

    public void StartDialogue(DialogueTree tree)
    {
        _tree = tree;
        if (!string.IsNullOrEmpty(tree.StartNodeId))
        {
            _currentNode = tree.GetNode(tree.StartNodeId);
            if (_currentNode != null)
                OnNodeChanged?.Invoke(_currentNode);
        }
    }

    public void SelectChoice(int index)
    {
        if (_currentNode == null || _tree == null) return;
        if (index < 0 || index >= _currentNode.Choices.Count) return;

        var choice = _currentNode.Choices[index];
        NavigateTo(choice.NextNodeId);
    }

    public void Advance()
    {
        if (_currentNode == null || _tree == null) return;
        if (_currentNode.Choices.Count > 0) return;
        NavigateTo(_currentNode.NextNodeId);
    }

    public void EndDialogue()
    {
        _tree = null;
        _currentNode = null;
        OnDialogueEnd?.Invoke();
    }

    public static bool EvaluateCondition(string condition, DialogueDatabase db)
    {
        if (string.IsNullOrWhiteSpace(condition)) return true;

        var parts = condition.Split("==", 2, StringSplitOptions.TrimEntries);
        if (parts.Length != 2) return true;

        string varValue = db.GetVariable(parts[0]);
        return string.Equals(varValue, parts[1], StringComparison.OrdinalIgnoreCase);
    }

    private void NavigateTo(string nodeId)
    {
        if (_tree == null) { EndDialogue(); return; }
        if (string.IsNullOrEmpty(nodeId)) { EndDialogue(); return; }

        var next = _tree.GetNode(nodeId);
        if (next == null) { EndDialogue(); return; }

        _currentNode = next;
        OnNodeChanged?.Invoke(_currentNode);
    }
}
