namespace InputMapper;

public enum InputSource
{
    Keyboard,
    Mouse
}

public class InputBinding
{
    public InputSource Source { get; set; }
    public string KeyName { get; set; }
    public string? Modifier { get; set; }

    public InputBinding(InputSource source, string keyName, string? modifier = null)
    {
        Source = source;
        KeyName = keyName;
        Modifier = modifier;
    }

    public InputBinding Clone() => new(Source, KeyName, Modifier);

    public string DisplayString()
    {
        string mod = string.IsNullOrEmpty(Modifier) ? "" : $"{Modifier}+";
        return $"{mod}{KeyName}";
    }

    public bool Equals(InputBinding? other)
    {
        if (other == null) return false;
        return Source == other.Source && KeyName == other.KeyName && Modifier == other.Modifier;
    }
}

public class InputAction
{
    public string Id { get; set; }
    public string Name { get; set; }
    public string Category { get; set; }
    public InputBinding PrimaryBinding { get; set; }
    public InputBinding? SecondaryBinding { get; set; }
    public string Description { get; set; }

    public InputAction(string id, string name, string category, InputBinding primary, string description = "")
    {
        Id = id;
        Name = name;
        Category = category;
        PrimaryBinding = primary;
        Description = description;
    }

    public InputAction Clone()
    {
        return new InputAction(Id, Name, Category, PrimaryBinding.Clone(), Description)
        {
            SecondaryBinding = SecondaryBinding?.Clone()
        };
    }
}

public class InputProfile
{
    public string Name { get; set; }
    public List<InputAction> Actions { get; set; }
    public bool IsDefault { get; set; }

    public InputProfile(string name, bool isDefault = false)
    {
        Name = name;
        Actions = new List<InputAction>();
        IsDefault = isDefault;
    }

    public InputProfile Clone(string newName)
    {
        var clone = new InputProfile(newName);
        foreach (var action in Actions)
            clone.Actions.Add(action.Clone());
        return clone;
    }
}

public class InputActionMap
{
    public List<InputProfile> Profiles { get; set; } = new();
    public InputProfile? ActiveProfile { get; private set; }

    public void AddProfile(string name)
    {
        if (Profiles.Any(p => p.Name == name)) return;
        var profile = new InputProfile(name);
        Profiles.Add(profile);
        if (ActiveProfile == null) ActiveProfile = profile;
    }

    public void RemoveProfile(string name)
    {
        var profile = Profiles.FirstOrDefault(p => p.Name == name);
        if (profile == null || profile.IsDefault) return;
        Profiles.Remove(profile);
        if (ActiveProfile == profile)
            ActiveProfile = Profiles.FirstOrDefault();
    }

    public void SetActiveProfile(string name)
    {
        var profile = Profiles.FirstOrDefault(p => p.Name == name);
        if (profile != null) ActiveProfile = profile;
    }

    public InputAction? GetAction(string actionId)
    {
        return ActiveProfile?.Actions.FirstOrDefault(a => a.Id == actionId);
    }

    public void RebindAction(string actionId, InputBinding newBinding, bool isPrimary)
    {
        var action = GetAction(actionId);
        if (action == null) return;
        if (isPrimary)
            action.PrimaryBinding = newBinding;
        else
            action.SecondaryBinding = newBinding;
    }

    public void ResetToDefault()
    {
        var defaultProfile = Profiles.FirstOrDefault(p => p.IsDefault);
        if (defaultProfile == null || ActiveProfile == null) return;
        ActiveProfile.Actions.Clear();
        foreach (var action in defaultProfile.Actions)
            ActiveProfile.Actions.Add(action.Clone());
    }

    public string? HasConflict(InputBinding binding, string excludeActionId)
    {
        if (ActiveProfile == null) return null;
        foreach (var action in ActiveProfile.Actions)
        {
            if (action.Id == excludeActionId) continue;
            if (action.PrimaryBinding.Equals(binding))
                return action.Name;
            if (action.SecondaryBinding != null && action.SecondaryBinding.Equals(binding))
                return action.Name;
        }
        return null;
    }
}
