namespace QuestSystem;

public class QuestManager
{
    private static QuestManager? _instance;
    public static QuestManager Instance => _instance ??= new QuestManager();

    public QuestDatabase Database { get; } = new();

    public event Action<Quest>? OnQuestStarted;
    public event Action<Quest>? OnQuestCompleted;
    public event Action<Quest>? OnQuestFailed;
    public event Action<Quest, QuestObjective>? OnObjectiveUpdated;
    public event Action<Achievement>? OnAchievementUnlocked;

    public bool StartQuest(string questId)
    {
        var quest = Database.GetQuest(questId);
        if (quest == null) return false;
        if (quest.Status == QuestStatus.InProgress) return false;
        if (!CheckPrerequisites(questId)) return false;

        quest.Status = QuestStatus.InProgress;
        if (quest.IsRepeatable)
        {
            foreach (var obj in quest.Objectives)
                obj.CurrentCount = 0;
        }
        OnQuestStarted?.Invoke(quest);
        return true;
    }

    public void UpdateObjective(string questId, string objectiveId, int amount)
    {
        var quest = Database.GetQuest(questId);
        if (quest == null || quest.Status != QuestStatus.InProgress) return;

        var objective = quest.Objectives.Find(o => o.Id == objectiveId);
        if (objective == null || objective.IsCompleted) return;

        objective.CurrentCount = Math.Min(objective.CurrentCount + amount, objective.RequiredCount);
        OnObjectiveUpdated?.Invoke(quest, objective);

        if (quest.AllObjectivesComplete)
            CompleteQuest(questId);
    }

    public void CompleteQuest(string questId)
    {
        var quest = Database.GetQuest(questId);
        if (quest == null) return;
        quest.Status = QuestStatus.Completed;
        OnQuestCompleted?.Invoke(quest);
    }

    public void FailQuest(string questId)
    {
        var quest = Database.GetQuest(questId);
        if (quest == null) return;
        quest.Status = QuestStatus.Failed;
        OnQuestFailed?.Invoke(quest);
    }

    public QuestStatus GetQuestStatus(string questId)
    {
        var quest = Database.GetQuest(questId);
        return quest?.Status ?? QuestStatus.NotStarted;
    }

    public bool CheckPrerequisites(string questId)
    {
        var quest = Database.GetQuest(questId);
        if (quest == null) return false;
        if (string.IsNullOrEmpty(quest.PrerequisiteQuestId)) return true;

        var prereq = Database.GetQuest(quest.PrerequisiteQuestId);
        return prereq?.Status == QuestStatus.Completed;
    }

    public float GetCompletionRate()
    {
        if (Database.Quests.Count == 0) return 0;
        int completed = Database.Quests.Count(q => q.Status == QuestStatus.Completed);
        return (float)completed / Database.Quests.Count * 100f;
    }
}
