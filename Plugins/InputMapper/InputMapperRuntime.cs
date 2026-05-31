namespace InputMapper;

public class InputMapperManager
{
    private static InputMapperManager? _instance;
    public static InputMapperManager Instance => _instance ??= new InputMapperManager();

    public InputActionMap ActionMap { get; } = new();
    public event Action<string>? OnActionTriggered;

    private InputMapperManager()
    {
        InitDefaultProfile();
    }

    public bool IsActionPressed(string actionId)
    {
        // Placeholder: in a real implementation this would check actual input state
        return false;
    }

    public bool IsActionDown(string actionId)
    {
        return false;
    }

    public bool IsActionUp(string actionId)
    {
        return false;
    }

    public void InitDefaultProfile()
    {
        var profile = new InputProfile("Default", isDefault: true);

        profile.Actions.Add(new InputAction("move_up", "向上移动", "移动",
            new InputBinding(InputSource.Keyboard, "W"), "向上移动角色"));
        profile.Actions.Add(new InputAction("move_down", "向下移动", "移动",
            new InputBinding(InputSource.Keyboard, "S"), "向下移动角色"));
        profile.Actions.Add(new InputAction("move_left", "向左移动", "移动",
            new InputBinding(InputSource.Keyboard, "A"), "向左移动角色"));
        profile.Actions.Add(new InputAction("move_right", "向右移动", "移动",
            new InputBinding(InputSource.Keyboard, "D"), "向右移动角色"));
        profile.Actions.Add(new InputAction("jump", "跳跃", "动作",
            new InputBinding(InputSource.Keyboard, "Space"), "角色跳跃"));
        profile.Actions.Add(new InputAction("attack", "攻击", "动作",
            new InputBinding(InputSource.Mouse, "LeftMouse"), "主攻击"));
        profile.Actions.Add(new InputAction("interact", "交互", "动作",
            new InputBinding(InputSource.Keyboard, "E"), "与物体交互"));
        profile.Actions.Add(new InputAction("pause", "暂停", "系统",
            new InputBinding(InputSource.Keyboard, "Escape"), "暂停游戏"));
        profile.Actions.Add(new InputAction("inventory", "背包", "UI",
            new InputBinding(InputSource.Keyboard, "I"), "打开背包"));
        profile.Actions.Add(new InputAction("map", "地图", "UI",
            new InputBinding(InputSource.Keyboard, "M"), "打开地图"));

        ActionMap.Profiles.Add(profile);
        var userProfile = profile.Clone("自定义");
        ActionMap.Profiles.Add(userProfile);
        ActionMap.SetActiveProfile("自定义");
    }
}
