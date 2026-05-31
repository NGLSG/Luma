using Luma.SDK.Plugins;

namespace InventorySystem;

public class InventoryPlugin : EditorPlugin
{
    public override string Id => "com.luma.inventory";
    public override string Name => "背包系统";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "RPG/生存游戏背包与物品管理系统。";

    private ItemDatabasePanel? _dbPanel;
    private InventoryPreviewPanel? _previewPanel;
    private readonly ItemDatabase _sharedDb = new();

    public override void OnLoad()
    {
        base.OnLoad();
        _dbPanel = RegisterPanel<ItemDatabasePanel>();
        _previewPanel = RegisterPanel<InventoryPreviewPanel>();
        _dbPanel.Database = _sharedDb;
        _previewPanel.Database = _sharedDb;
        RegisterSampleItems();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("背包"))
        {
            if (ImGui.MenuItem("物品数据库"))
            {
                if (_dbPanel != null) _dbPanel.IsVisible = true;
            }
            if (ImGui.MenuItem("背包预览"))
            {
                if (_previewPanel != null) _previewPanel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "项目")
        {
            if (ImGui.MenuItem("背包系统"))
            {
                if (_dbPanel != null) _dbPanel.IsVisible = true;
            }
        }
    }

    private void RegisterSampleItems()
    {
        _sharedDb.RegisterItem(new ItemDefinition { Id = "potion_hp", Name = "生命药水", Description = "恢复50点生命值", Type = ItemType.Consumable, Rarity = ItemRarity.Common, MaxStackSize = 20, IsUsable = true });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "potion_mp", Name = "魔力药水", Description = "恢复30点魔力", Type = ItemType.Consumable, Rarity = ItemRarity.Common, MaxStackSize = 20, IsUsable = true });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "sword_iron", Name = "铁剑", Description = "普通的铁质长剑", Type = ItemType.Equipment, Rarity = ItemRarity.Common, MaxStackSize = 1 });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "sword_flame", Name = "烈焰之刃", Description = "附魔火焰的传说之剑", Type = ItemType.Equipment, Rarity = ItemRarity.Legendary, MaxStackSize = 1 });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "ore_iron", Name = "铁矿石", Description = "常见的铁矿石", Type = ItemType.Material, Rarity = ItemRarity.Common, MaxStackSize = 99 });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "gem_ruby", Name = "红宝石", Description = "闪烁着深红色光芒的宝石", Type = ItemType.Material, Rarity = ItemRarity.Rare, MaxStackSize = 10 });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "quest_letter", Name = "神秘信件", Description = "一封来自远方的密信", Type = ItemType.Quest, Rarity = ItemRarity.Uncommon, MaxStackSize = 1, IsTradable = false });
        _sharedDb.RegisterItem(new ItemDefinition { Id = "armor_plate", Name = "板甲", Description = "厚重的板甲，提供极高防御", Type = ItemType.Equipment, Rarity = ItemRarity.Epic, MaxStackSize = 1 });
    }
}

public class ItemDatabasePanel : EditorPanel
{
    public override string Title => "物品数据库";

    public ItemDatabase Database { get; set; } = new();

    private ItemDefinition? _selectedItem;
    private string _searchFilter = "";
    private int _typeFilter = -1;
    private int _rarityFilter = -1;

    private string _newId = "";
    private string _newName = "";
    private string _newDesc = "";
    private int _newType;
    private int _newRarity;
    private int _newMaxStack = 99;
    private bool _newUsable;
    private bool _newTradable = true;

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("ItemDbTabs"))
        {
            if (ImGui.BeginTabItem("物品列表"))
            {
                DrawItemList();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("物品编辑"))
            {
                DrawItemEditor();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("新建物品"))
            {
                DrawNewItemForm();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawItemList()
    {
        ImGui.InputText("搜索", ref _searchFilter);
        ImGui.SameLine();
        ImGui.Text($"共 {Database.Items.Count} 个物品");

        ImGui.Separator();

        var items = Database.GetAllItems().AsEnumerable();

        if (!string.IsNullOrEmpty(_searchFilter))
            items = items.Where(i => i.Name.Contains(_searchFilter, StringComparison.OrdinalIgnoreCase)
                                  || i.Id.Contains(_searchFilter, StringComparison.OrdinalIgnoreCase));
        if (_typeFilter >= 0)
            items = items.Where(i => (int)i.Type == _typeFilter);
        if (_rarityFilter >= 0)
            items = items.Where(i => (int)i.Rarity == _rarityFilter);

        DrawFilterButtons();
        ImGui.Separator();

        if (ImGui.BeginTable("ItemTable", 5))
        {
            ImGui.TableSetupColumn("ID");
            ImGui.TableSetupColumn("名称");
            ImGui.TableSetupColumn("类型");
            ImGui.TableSetupColumn("稀有度");
            ImGui.TableSetupColumn("堆叠");
            ImGui.TableHeadersRow();

            foreach (var item in items)
            {
                ImGui.TableNextRow();

                ImGui.TableNextColumn();
                ImGui.PushID($"item_{item.Id}");
                if (ImGui.Selectable(item.Id, _selectedItem == item))
                    _selectedItem = item;
                ImGui.PopID();

                ImGui.TableNextColumn();
                var (r, g, b) = GetRarityColor(item.Rarity);
                ImGui.TextColored(r, g, b, 1.0f, item.Name);

                ImGui.TableNextColumn();
                ImGui.Text(GetTypeName(item.Type));

                ImGui.TableNextColumn();
                ImGui.TextColored(r, g, b, 1.0f, GetRarityName(item.Rarity));

                ImGui.TableNextColumn();
                ImGui.Text($"{item.MaxStackSize}");
            }
            ImGui.EndTable();
        }
    }

    private void DrawFilterButtons()
    {
        ImGui.Text("类型:");
        ImGui.SameLine();
        if (ImGui.SmallButton("全部##type")) _typeFilter = -1;
        string[] typeNames = { "消耗品", "装备", "材料", "任务", "杂项" };
        for (int i = 0; i < typeNames.Length; i++)
        {
            ImGui.SameLine();
            ImGui.PushID($"tf_{i}");
            if (ImGui.SmallButton(_typeFilter == i ? $"[{typeNames[i]}]" : typeNames[i]))
                _typeFilter = _typeFilter == i ? -1 : i;
            ImGui.PopID();
        }

        ImGui.Text("稀有度:");
        ImGui.SameLine();
        if (ImGui.SmallButton("全部##rarity")) _rarityFilter = -1;
        string[] rarityNames = { "普通", "优秀", "稀有", "史诗", "传说" };
        for (int i = 0; i < rarityNames.Length; i++)
        {
            ImGui.SameLine();
            ImGui.PushID($"rf_{i}");
            if (ImGui.SmallButton(_rarityFilter == i ? $"[{rarityNames[i]}]" : rarityNames[i]))
                _rarityFilter = _rarityFilter == i ? -1 : i;
            ImGui.PopID();
        }
    }

    private void DrawItemEditor()
    {
        if (_selectedItem == null)
        {
            ImGui.TextDisabled("请在物品列表中选择一个物品");
            return;
        }

        var (r, g, b) = GetRarityColor(_selectedItem.Rarity);
        ImGui.TextColored(r, g, b, 1.0f, $"编辑: {_selectedItem.Name}");
        ImGui.Separator();

        ImGui.Text($"ID: {_selectedItem.Id}");

        var name = _selectedItem.Name;
        if (ImGui.InputText("名称", ref name))
            _selectedItem.Name = name;

        var desc = _selectedItem.Description;
        if (ImGui.InputText("描述", ref desc))
            _selectedItem.Description = desc;

        ImGui.Separator();
        ImGui.Text("类型:");
        for (int i = 0; i < 5; i++)
        {
            ImGui.PushID($"etype_{i}");
            if (ImGui.Selectable(GetTypeName((ItemType)i), _selectedItem.Type == (ItemType)i))
                _selectedItem.Type = (ItemType)i;
            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.Text("稀有度:");
        for (int i = 0; i < 5; i++)
        {
            var (cr, cg, cb) = GetRarityColor((ItemRarity)i);
            ImGui.PushID($"erarity_{i}");
            if (ImGui.Selectable(GetRarityName((ItemRarity)i), _selectedItem.Rarity == (ItemRarity)i))
                _selectedItem.Rarity = (ItemRarity)i;
            ImGui.PopID();
        }

        ImGui.Separator();
        var maxStack = _selectedItem.MaxStackSize;
        if (ImGui.InputInt("堆叠上限", ref maxStack))
            _selectedItem.MaxStackSize = Math.Max(1, maxStack);

        var usable = _selectedItem.IsUsable;
        if (ImGui.Checkbox("可使用", ref usable))
            _selectedItem.IsUsable = usable;

        var tradable = _selectedItem.IsTradable;
        if (ImGui.Checkbox("可交易", ref tradable))
            _selectedItem.IsTradable = tradable;

        ImGui.Separator();
        if (ImGui.CollapsingHeader("自定义属性"))
        {
            DrawPropertyEditor(_selectedItem);
        }
    }

    private void DrawPropertyEditor(ItemDefinition item)
    {
        string? propToRemove = null;
        foreach (var kvp in item.Properties)
        {
            ImGui.PushID($"prop_{kvp.Key}");
            ImGui.Text(kvp.Key);
            ImGui.SameLine();
            var val = kvp.Value;
            if (ImGui.InputText("##val", ref val))
                item.Properties[kvp.Key] = val;
            ImGui.SameLine();
            if (ImGui.SmallButton("X"))
                propToRemove = kvp.Key;
            ImGui.PopID();
        }
        if (propToRemove != null)
            item.Properties.Remove(propToRemove);

        ImGui.Spacing();
        if (ImGui.Button("+ 添加属性"))
        {
            string key = $"prop_{item.Properties.Count}";
            item.Properties[key] = "";
        }
    }

    private void DrawNewItemForm()
    {
        ImGui.Text("创建新物品");
        ImGui.Separator();

        ImGui.InputText("ID", ref _newId);
        ImGui.InputText("名称", ref _newName);
        ImGui.InputText("描述", ref _newDesc);

        ImGui.Separator();
        ImGui.Text("类型:");
        for (int i = 0; i < 5; i++)
        {
            ImGui.PushID($"ntype_{i}");
            if (ImGui.Selectable(GetTypeName((ItemType)i), _newType == i))
                _newType = i;
            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.Text("稀有度:");
        for (int i = 0; i < 5; i++)
        {
            ImGui.PushID($"nrarity_{i}");
            if (ImGui.Selectable(GetRarityName((ItemRarity)i), _newRarity == i))
                _newRarity = i;
            ImGui.PopID();
        }

        ImGui.Separator();
        ImGui.InputInt("堆叠上限", ref _newMaxStack);
        ImGui.Checkbox("可使用", ref _newUsable);
        ImGui.Checkbox("可交易", ref _newTradable);

        ImGui.Separator();
        if (ImGui.Button("创建物品"))
        {
            if (!string.IsNullOrWhiteSpace(_newId) && !string.IsNullOrWhiteSpace(_newName))
            {
                Database.RegisterItem(new ItemDefinition
                {
                    Id = _newId,
                    Name = _newName,
                    Description = _newDesc,
                    Type = (ItemType)_newType,
                    Rarity = (ItemRarity)_newRarity,
                    MaxStackSize = Math.Max(1, _newMaxStack),
                    IsUsable = _newUsable,
                    IsTradable = _newTradable
                });
                Log.Info($"[背包系统] 创建物品: {_newName} ({_newId})");
                _newId = "";
                _newName = "";
                _newDesc = "";
            }
        }
    }

    private static (float r, float g, float b) GetRarityColor(ItemRarity rarity) => rarity switch
    {
        ItemRarity.Common    => (0.9f, 0.9f, 0.9f),
        ItemRarity.Uncommon  => (0.2f, 0.8f, 0.2f),
        ItemRarity.Rare      => (0.3f, 0.5f, 1.0f),
        ItemRarity.Epic      => (0.7f, 0.3f, 0.9f),
        ItemRarity.Legendary => (1.0f, 0.8f, 0.1f),
        _ => (0.9f, 0.9f, 0.9f)
    };

    private static string GetRarityName(ItemRarity rarity) => rarity switch
    {
        ItemRarity.Common    => "普通",
        ItemRarity.Uncommon  => "优秀",
        ItemRarity.Rare      => "稀有",
        ItemRarity.Epic      => "史诗",
        ItemRarity.Legendary => "传说",
        _ => "未知"
    };

    private static string GetTypeName(ItemType type) => type switch
    {
        ItemType.Consumable => "消耗品",
        ItemType.Equipment  => "装备",
        ItemType.Material   => "材料",
        ItemType.Quest      => "任务",
        ItemType.Misc       => "杂项",
        _ => "未知"
    };
}

public class InventoryPreviewPanel : EditorPanel
{
    public override string Title => "背包预览";

    public ItemDatabase Database { get; set; } = new();

    private readonly Inventory _inventory = new(20);
    private readonly EquipmentManager _equipment = new();
    private string _addItemId = "";
    private int _addCount = 1;

    public override void OnGUI()
    {
        if (ImGui.BeginTabBar("InvPreviewTabs"))
        {
            if (ImGui.BeginTabItem("背包"))
            {
                DrawInventoryGrid();
                ImGui.EndTabItem();
            }
            if (ImGui.BeginTabItem("装备栏"))
            {
                DrawEquipment();
                ImGui.EndTabItem();
            }
            ImGui.EndTabBar();
        }
    }

    private void DrawInventoryGrid()
    {
        ImGui.Text("背包管理");
        ImGui.Separator();

        ImGui.Text($"槽位: {_inventory.GetUsedSlots()}/{_inventory.Capacity}");
        float usage = _inventory.Capacity > 0 ? (float)_inventory.GetUsedSlots() / _inventory.Capacity : 0;
        ImGui.ProgressBar(usage, $"{_inventory.GetUsedSlots()}/{_inventory.Capacity}");

        ImGui.Separator();

        ImGui.InputText("物品ID", ref _addItemId);
        ImGui.SameLine();
        ImGui.InputInt("数量", ref _addCount);
        ImGui.SameLine();
        if (ImGui.Button("添加"))
        {
            if (!string.IsNullOrWhiteSpace(_addItemId) && _addCount > 0)
            {
                bool ok = _inventory.AddItem(_addItemId, _addCount, Database);
                if (!ok) Log.Info("[背包] 添加失败: 背包已满或物品不存在");
            }
        }

        ImGui.Separator();
        if (ImGui.Button("按类型排序")) _inventory.SortByType();
        ImGui.SameLine();
        if (ImGui.Button("按稀有度排序")) _inventory.SortByRarity();
        ImGui.SameLine();
        if (ImGui.Button("按名称排序")) _inventory.SortByName();
        ImGui.SameLine();
        if (ImGui.Button("清空")) _inventory.Clear();

        ImGui.Separator();

        if (_inventory.Slots.Count == 0)
        {
            ImGui.TextDisabled("背包为空");
            return;
        }

        int cols = 5;
        if (ImGui.BeginTable("InvGrid", cols))
        {
            for (int i = 0; i < cols; i++)
                ImGui.TableSetupColumn($"C{i}");

            for (int i = 0; i < _inventory.Capacity; i++)
            {
                if (i % cols == 0) ImGui.TableNextRow();
                ImGui.TableNextColumn();

                ImGui.PushID($"slot_{i}");
                if (i < _inventory.Slots.Count)
                {
                    var stack = _inventory.Slots[i];
                    var (r, g, b) = GetRarityColor(stack.Item.Rarity);
                    string label = $"{stack.Item.Name}\nx{stack.Count}";
                    if (ImGui.Button(label))
                    {
                        _inventory.RemoveItem(stack.Item.Id, 1);
                    }
                }
                else
                {
                    ImGui.TextDisabled("[空]");
                }
                ImGui.PopID();
            }
            ImGui.EndTable();
        }

        ImGui.TextDisabled("点击物品可移除1个");
    }

    private void DrawEquipment()
    {
        ImGui.Text("装备栏");
        ImGui.Separator();

        foreach (var slot in _equipment.Slots)
        {
            ImGui.PushID($"equip_{slot.SlotName}");

            string slotLabel = slot.SlotName switch
            {
                "Weapon"    => "武器",
                "Armor"     => "护甲",
                "Helmet"    => "头盔",
                "Boots"     => "鞋子",
                "Accessory" => "饰品",
                _ => slot.SlotName
            };

            if (slot.EquippedItem != null)
            {
                var (r, g, b) = GetRarityColor(slot.EquippedItem.Item.Rarity);
                ImGui.TextColored(r, g, b, 1.0f, $"{slotLabel}: {slot.EquippedItem.Item.Name}");
                ImGui.SameLine();
                if (ImGui.SmallButton("卸下"))
                {
                    _equipment.Unequip(slot.SlotName);
                }
            }
            else
            {
                ImGui.TextDisabled($"{slotLabel}: [空]");

                var equipItems = _inventory.Slots
                    .Where(s => s.Item.Type == ItemType.Equipment)
                    .ToList();

                if (equipItems.Count > 0)
                {
                    ImGui.SameLine();
                    if (ImGui.SmallButton("装备"))
                    {
                        var item = equipItems[0];
                        _equipment.Equip(slot.SlotName, item);
                        _inventory.RemoveItem(item.Item.Id, 1);
                    }
                }
            }
            ImGui.PopID();
        }
    }

    private static (float r, float g, float b) GetRarityColor(ItemRarity rarity) => rarity switch
    {
        ItemRarity.Common    => (0.9f, 0.9f, 0.9f),
        ItemRarity.Uncommon  => (0.2f, 0.8f, 0.2f),
        ItemRarity.Rare      => (0.3f, 0.5f, 1.0f),
        ItemRarity.Epic      => (0.7f, 0.3f, 0.9f),
        ItemRarity.Legendary => (1.0f, 0.8f, 0.1f),
        _ => (0.9f, 0.9f, 0.9f)
    };
}
