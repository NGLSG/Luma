using Luma.SDK.Plugins;
namespace QuestSystem;

public class QuestPlugin : EditorPlugin
{
    public override string Id => "com.luma.quest";
    public override string Name => "任务系统";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "RPG任务线与成就系统，支持多目标、分支、奖励";

    private QuestEditorPanel? _questPanel;
    private AchievementPanel? _achievementPanel;

    public override void OnLoad()
    {
        base.OnLoad();
        _questPanel = RegisterPanel<QuestEditorPanel>();
        _achievementPanel = RegisterPanel<AchievementPanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("任务"))
        {
            if (ImGui.MenuItem("任务编辑器"))
            {
                if (_questPanel != null)
                    _questPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("成就管理"))
            {
                if (_achievementPanel != null)
                    _achievementPanel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("任务系统"))
            {
                if (_questPanel != null)
                    _questPanel.IsVisible = true;
            }
        }
    }
}

public class QuestEditorPanel : EditorPanel
{
    public override string Title => "任务编辑器";

    private string _selectedQuestId = "";
    private string _searchFilter = "";
    private int _statusFilter = -1;

    private string _newQuestId = "";
    private string _newQuestName = "";
    private string _newQuestDesc = "";

    private string _newObjId = "";
    private string _newObjDesc = "";
    private int _newObjType = 4;
    private int _newObjRequired = 1;

    private string _newRewardType = "Gold";
    private string _newRewardValue = "";
    private int _newRewardAmount = 1;

    private string _newChainId = "";
    private string _newChainName = "";

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("QuestTabs"))
        {
            if (ImGui.BeginTabItem("任务列表"))
            {
                DrawQuestList();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("任务详情"))
            {
                DrawQuestDetail();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("新建任务"))
            {
                DrawNewQuest();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("任务链"))
            {
                DrawQuestChains();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawQuestList()
    {
        var mgr = QuestManager.Instance;
        var quests = mgr.Database.Quests;

        ImGui.Text("搜索:");
        ImGui.SameLine();
        ImGui.InputText("##QuestSearch", ref _searchFilter);

        ImGui.Text($"完成率: {mgr.GetCompletionRate():F1}%");
        ImGui.Separator();

        if (quests.Count == 0)
        {
            ImGui.TextDisabled("暂无任务，请在「新建任务」中创建");
            return;
        }

        if (ImGui.BeginTable("QuestTable", 5))
        {
            ImGui.TableSetupColumn("ID");
            ImGui.TableSetupColumn("名称");
            ImGui.TableSetupColumn("状态");
            ImGui.TableSetupColumn("目标数");
            ImGui.TableSetupColumn("完成度");
            ImGui.TableHeadersRow();

            for (int i = 0; i < quests.Count; i++)
            {
                var q = quests[i];

                if (!string.IsNullOrEmpty(_searchFilter) &&
                    !q.Name.Contains(_searchFilter, StringComparison.OrdinalIgnoreCase) &&
                    !q.Id.Contains(_searchFilter, StringComparison.OrdinalIgnoreCase))
                    continue;

                if (_statusFilter >= 0 && (int)q.Status != _statusFilter)
                    continue;

                ImGui.PushID(i);
                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                if (ImGui.Selectable(q.Id, q.Id == _selectedQuestId))
                    _selectedQuestId = q.Id;

                ImGui.TableNextColumn();
                ImGui.Text(q.Name);

                ImGui.TableNextColumn();
                switch (q.Status)
                {
                    case QuestStatus.NotStarted:
                        ImGui.TextDisabled("未开始");
                        break;
                    case QuestStatus.InProgress:
                        ImGui.TextColored(1.0f, 0.8f, 0.0f, 1.0f, "进行中");
                        break;
                    case QuestStatus.Completed:
                        ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "已完成");
                        break;
                    case QuestStatus.Failed:
                        ImGui.TextColored(0.8f, 0.2f, 0.2f, 1.0f, "已失败");
                        break;
                }

                ImGui.TableNextColumn();
                ImGui.Text($"{q.Objectives.Count}");

                ImGui.TableNextColumn();
                if (q.Objectives.Count > 0)
                {
                    int done = q.Objectives.Count(o => o.IsCompleted);
                    float pct = (float)done / q.Objectives.Count;
                    ImGui.ProgressBar(pct, $"{done}/{q.Objectives.Count}");
                }
                else
                {
                    ImGui.TextDisabled("-");
                }

                ImGui.PopID();
            }

            ImGui.EndTable();
        }
    }

    private void DrawQuestDetail()
    {
        var mgr = QuestManager.Instance;
        var quest = mgr.Database.GetQuest(_selectedQuestId);

        if (quest == null)
        {
            ImGui.TextDisabled("请在「任务列表」中选择一个任务");
            return;
        }

        ImGui.Text($"任务 ID: {quest.Id}");
        ImGui.Text($"名称: {quest.Name}");
        ImGui.Text($"描述: {quest.Description}");

        ImGui.Separator();
        switch (quest.Status)
        {
            case QuestStatus.NotStarted:
                ImGui.TextDisabled("状态: 未开始");
                if (ImGui.Button("开始任务"))
                    mgr.StartQuest(quest.Id);
                break;
            case QuestStatus.InProgress:
                ImGui.TextColored(1.0f, 0.8f, 0.0f, 1.0f, "状态: 进行中");
                if (ImGui.Button("完成"))
                    mgr.CompleteQuest(quest.Id);
                ImGui.SameLine();
                if (ImGui.Button("失败"))
                    mgr.FailQuest(quest.Id);
                break;
            case QuestStatus.Completed:
                ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "状态: 已完成");
                break;
            case QuestStatus.Failed:
                ImGui.TextColored(0.8f, 0.2f, 0.2f, 1.0f, "状态: 已失败");
                break;
        }

        if (!string.IsNullOrEmpty(quest.PrerequisiteQuestId))
            ImGui.Text($"前置任务: {quest.PrerequisiteQuestId}");

        ImGui.Text($"可重复: {(quest.IsRepeatable ? "是" : "否")}");

        ImGui.Separator();
        if (ImGui.CollapsingHeader($"目标 ({quest.Objectives.Count})"))
        {
            for (int i = 0; i < quest.Objectives.Count; i++)
            {
                var obj = quest.Objectives[i];
                ImGui.PushID(i + 1000);

                string status = obj.IsCompleted ? "[完成]" : $"[{obj.CurrentCount}/{obj.RequiredCount}]";
                if (obj.IsCompleted)
                    ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, $"  {status} {obj.Description} ({obj.Type})");
                else
                    ImGui.Text($"  {status} {obj.Description} ({obj.Type})");

                if (quest.Status == QuestStatus.InProgress && !obj.IsCompleted)
                {
                    ImGui.SameLine();
                    if (ImGui.SmallButton("+1"))
                        mgr.UpdateObjective(quest.Id, obj.Id, 1);
                }

                ImGui.PopID();
            }

            ImGui.Separator();
            ImGui.Text("添加目标:");
            ImGui.InputText("目标ID##NewObj", ref _newObjId);
            ImGui.InputText("描述##NewObj", ref _newObjDesc);
            ImGui.Text($"所需数量: {_newObjRequired}");
            if (ImGui.Button("添加目标"))
            {
                if (!string.IsNullOrWhiteSpace(_newObjId))
                {
                    quest.Objectives.Add(new QuestObjective
                    {
                        Id = _newObjId,
                        Description = _newObjDesc,
                        Type = (ObjectiveType)_newObjType,
                        RequiredCount = _newObjRequired
                    });
                    _newObjId = "";
                    _newObjDesc = "";
                }
            }
        }

        if (ImGui.CollapsingHeader($"奖励 ({quest.Rewards.Count})"))
        {
            for (int i = 0; i < quest.Rewards.Count; i++)
            {
                var r = quest.Rewards[i];
                ImGui.Text($"  {r.Type}: {r.Value} x{r.Amount}");
            }

            ImGui.Separator();
            ImGui.Text("添加奖励:");
            ImGui.InputText("类型##Reward", ref _newRewardType);
            ImGui.InputText("值##Reward", ref _newRewardValue);
            ImGui.Text($"数量: {_newRewardAmount}");
            if (ImGui.Button("添加奖励"))
            {
                quest.Rewards.Add(new QuestReward
                {
                    Type = _newRewardType,
                    Value = _newRewardValue,
                    Amount = _newRewardAmount
                });
                _newRewardType = "Gold";
                _newRewardValue = "";
                _newRewardAmount = 1;
            }
        }

        ImGui.Separator();
        if (ImGui.Button("删除此任务"))
        {
            mgr.Database.RemoveQuest(quest.Id);
            _selectedQuestId = "";
        }
    }

    private void DrawNewQuest()
    {
        ImGui.Text("新建任务");
        ImGui.Separator();

        ImGui.InputText("任务 ID", ref _newQuestId);
        ImGui.InputText("名称", ref _newQuestName);
        ImGui.InputText("描述", ref _newQuestDesc);

        ImGui.Separator();
        if (ImGui.Button("创建任务"))
        {
            if (!string.IsNullOrWhiteSpace(_newQuestId) && !string.IsNullOrWhiteSpace(_newQuestName))
            {
                var quest = new Quest
                {
                    Id = _newQuestId,
                    Name = _newQuestName,
                    Description = _newQuestDesc
                };
                QuestManager.Instance.Database.AddQuest(quest);
                _selectedQuestId = quest.Id;
                _newQuestId = "";
                _newQuestName = "";
                _newQuestDesc = "";
                Log.Info($"创建任务: {quest.Name}");
            }
        }
    }

    private void DrawQuestChains()
    {
        var db = QuestManager.Instance.Database;

        ImGui.Text("任务链");
        ImGui.Separator();

        if (db.Chains.Count == 0)
        {
            ImGui.TextDisabled("暂无任务链");
        }
        else
        {
            for (int i = 0; i < db.Chains.Count; i++)
            {
                var chain = db.Chains[i];
                ImGui.PushID(i + 2000);

                if (ImGui.CollapsingHeader($"{chain.Name} ({chain.QuestIds.Count} 个任务)"))
                {
                    for (int j = 0; j < chain.QuestIds.Count; j++)
                    {
                        var qid = chain.QuestIds[j];
                        var q = db.GetQuest(qid);
                        string label = q != null ? $"  {j + 1}. {q.Name} [{q.Status}]" : $"  {j + 1}. {qid} [未找到]";
                        ImGui.Text(label);
                    }
                }

                ImGui.PopID();
            }
        }

        ImGui.Separator();
        ImGui.Text("新建任务链:");
        ImGui.InputText("链 ID", ref _newChainId);
        ImGui.InputText("链名称", ref _newChainName);
        if (ImGui.Button("创建任务链"))
        {
            if (!string.IsNullOrWhiteSpace(_newChainId) && !string.IsNullOrWhiteSpace(_newChainName))
            {
                db.Chains.Add(new QuestChain { Id = _newChainId, Name = _newChainName });
                _newChainId = "";
                _newChainName = "";
            }
        }
    }
}

public class AchievementPanel : EditorPanel
{
    public override string Title => "成就";

    private string _newAchId = "";
    private string _newAchName = "";
    private string _newAchDesc = "";
    private string _newAchIcon = "★";

    public override void OnGUI()
    {
        var db = QuestManager.Instance.Database;
        var achievements = db.Achievements;

        int total = achievements.Count;
        int unlocked = achievements.Count(a => a.IsUnlocked);
        float rate = total > 0 ? (float)unlocked / total * 100f : 0;

        ImGui.Text($"成就: {unlocked}/{total}  完成率: {rate:F1}%");
        if (total > 0)
            ImGui.ProgressBar(unlocked / (float)total, $"{unlocked}/{total}");

        ImGui.Separator();

        if (achievements.Count == 0)
        {
            ImGui.TextDisabled("暂无成就");
        }
        else if (ImGui.BeginTable("AchTable", 4))
        {
            ImGui.TableSetupColumn("图标");
            ImGui.TableSetupColumn("名称");
            ImGui.TableSetupColumn("描述");
            ImGui.TableSetupColumn("状态");
            ImGui.TableHeadersRow();

            for (int i = 0; i < achievements.Count; i++)
            {
                var ach = achievements[i];
                ImGui.PushID(i + 3000);

                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                ImGui.Text(ach.Icon);

                ImGui.TableNextColumn();
                ImGui.Text(ach.Name);

                ImGui.TableNextColumn();
                ImGui.TextDisabled(ach.Description);

                ImGui.TableNextColumn();
                if (ach.IsUnlocked)
                {
                    ImGui.TextColored(0.2f, 0.8f, 0.2f, 1.0f, "已解锁");
                }
                else
                {
                    if (ImGui.SmallButton("解锁"))
                    {
                        db.UnlockAchievement(ach.Id);
                        QuestManager.Instance.OnAchievementUnlocked?.Invoke(ach);
                        Log.Info($"解锁成就: {ach.Name}");
                    }
                }

                ImGui.PopID();
            }

            ImGui.EndTable();
        }

        ImGui.Separator();
        if (ImGui.CollapsingHeader("新建成就"))
        {
            ImGui.InputText("ID##NewAch", ref _newAchId);
            ImGui.InputText("名称##NewAch", ref _newAchName);
            ImGui.InputText("描述##NewAch", ref _newAchDesc);
            ImGui.InputText("图标##NewAch", ref _newAchIcon);
            if (ImGui.Button("创建成就"))
            {
                if (!string.IsNullOrWhiteSpace(_newAchId) && !string.IsNullOrWhiteSpace(_newAchName))
                {
                    db.AddAchievement(new Achievement
                    {
                        Id = _newAchId,
                        Name = _newAchName,
                        Description = _newAchDesc,
                        Icon = string.IsNullOrEmpty(_newAchIcon) ? "★" : _newAchIcon
                    });
                    _newAchId = "";
                    _newAchName = "";
                    _newAchDesc = "";
                    _newAchIcon = "★";
                    Log.Info("成就已创建");
                }
            }
        }
    }
}
