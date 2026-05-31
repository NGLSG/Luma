namespace QuestSystem;

public enum QuestStatus { NotStarted, InProgress, Completed, Failed }
public enum ObjectiveType { Kill, Collect, Talk, Explore, Custom }

public class QuestObjective
{
    public string Id { get; set; } = "";
    public string Description { get; set; } = "";
    public ObjectiveType Type { get; set; } = ObjectiveType.Custom;
    public int RequiredCount { get; set; } = 1;
    public int CurrentCount { get; set; } = 0;
    public bool IsCompleted => CurrentCount >= RequiredCount;
}

public class QuestReward
{
    public string Type { get; set; } = "Gold";
    public string Value { get; set; } = "";
    public int Amount { get; set; } = 0;
}

public class Quest
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public string Description { get; set; } = "";
    public QuestStatus Status { get; set; } = QuestStatus.NotStarted;
    public List<QuestObjective> Objectives { get; set; } = new();
    public List<QuestReward> Rewards { get; set; } = new();
    public string? PrerequisiteQuestId { get; set; }
    public bool IsRepeatable { get; set; } = false;
    public bool AllObjectivesComplete => Objectives.All(o => o.IsCompleted);
}

public class QuestChain
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public List<string> QuestIds { get; set; } = new();
}

public class Achievement
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public string Description { get; set; } = "";
    public bool IsUnlocked { get; set; } = false;
    public DateTime? UnlockedAt { get; set; }
    public string Icon { get; set; } = "★";
}

public class QuestDatabase
{
    public List<Quest> Quests { get; set; } = new();
    public List<QuestChain> Chains { get; set; } = new();
    public List<Achievement> Achievements { get; set; } = new();

    public void AddQuest(Quest quest) => Quests.Add(quest);

    public void RemoveQuest(string id) => Quests.RemoveAll(q => q.Id == id);

    public Quest? GetQuest(string id) => Quests.Find(q => q.Id == id);

    public void AddAchievement(Achievement ach) => Achievements.Add(ach);

    public void UnlockAchievement(string id)
    {
        var ach = Achievements.Find(a => a.Id == id);
        if (ach != null && !ach.IsUnlocked)
        {
            ach.IsUnlocked = true;
            ach.UnlockedAt = DateTime.Now;
        }
    }

    public List<Quest> GetActiveQuests() => Quests.Where(q => q.Status == QuestStatus.InProgress).ToList();

    public List<Quest> GetCompletedQuests() => Quests.Where(q => q.Status == QuestStatus.Completed).ToList();
}
