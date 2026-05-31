namespace SaveSystem;

public class SaveSlot
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public DateTime CreatedAt { get; set; }
    public DateTime ModifiedAt { get; set; }
    public TimeSpan PlayTime { get; set; }
    public string SceneName { get; set; } = "";
    public Dictionary<string, string> CustomData { get; set; } = new();
}

public class SaveProfile
{
    public string ProfileName { get; set; } = "Default";
    public List<SaveSlot> Slots { get; set; } = new();
    public int MaxSlots { get; set; } = 20;
    public string AutoSaveSlotId { get; set; } = "";
}

public class SaveEntry
{
    public string Key { get; set; } = "";
    public string Value { get; set; } = "";
    public string Type { get; set; } = "string";
}

public class SaveManager
{
    private static SaveManager? _instance;
    public static SaveManager Instance => _instance ??= new SaveManager();

    public SaveProfile CurrentProfile { get; private set; } = new();

    public SaveSlot CreateSlot(string name)
    {
        var slot = new SaveSlot
        {
            Id = Guid.NewGuid().ToString("N")[..8],
            Name = name,
            CreatedAt = DateTime.Now,
            ModifiedAt = DateTime.Now,
            PlayTime = TimeSpan.Zero,
            SceneName = ""
        };
        CurrentProfile.Slots.Add(slot);
        return slot;
    }

    public void DeleteSlot(string id)
    {
        CurrentProfile.Slots.RemoveAll(s => s.Id == id);
        if (CurrentProfile.AutoSaveSlotId == id)
            CurrentProfile.AutoSaveSlotId = "";
    }

    public void SaveToSlot(string slotId)
    {
        var slot = CurrentProfile.Slots.Find(s => s.Id == slotId);
        if (slot == null) return;
        slot.ModifiedAt = DateTime.Now;
        slot.SceneName = LumaAPI.GetCurrentSceneName();
    }

    public SaveSlot? LoadFromSlot(string slotId)
    {
        return CurrentProfile.Slots.Find(s => s.Id == slotId);
    }

    public void AutoSave()
    {
        if (string.IsNullOrEmpty(CurrentProfile.AutoSaveSlotId))
        {
            var slot = CreateSlot("AutoSave");
            CurrentProfile.AutoSaveSlotId = slot.Id;
        }
        SaveToSlot(CurrentProfile.AutoSaveSlotId);
    }

    public List<SaveSlot> GetAllSlots() => CurrentProfile.Slots;

    public void SetCustomData(string slotId, string key, string value)
    {
        var slot = CurrentProfile.Slots.Find(s => s.Id == slotId);
        if (slot == null) return;
        slot.CustomData[key] = value;
        slot.ModifiedAt = DateTime.Now;
    }

    public string GetCustomData(string slotId, string key)
    {
        var slot = CurrentProfile.Slots.Find(s => s.Id == slotId);
        if (slot == null) return "";
        return slot.CustomData.TryGetValue(key, out var value) ? value : "";
    }

    public string ExportSave(string slotId)
    {
        var slot = CurrentProfile.Slots.Find(s => s.Id == slotId);
        if (slot == null) return "{}";
        return System.Text.Json.JsonSerializer.Serialize(slot);
    }

    public SaveSlot? ImportSave(string json)
    {
        try
        {
            var slot = System.Text.Json.JsonSerializer.Deserialize<SaveSlot>(json);
            if (slot != null)
            {
                slot.Id = Guid.NewGuid().ToString("N")[..8];
                CurrentProfile.Slots.Add(slot);
            }
            return slot;
        }
        catch
        {
            return null;
        }
    }
}
