namespace InventorySystem;

public enum ItemRarity
{
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary
}

public enum ItemType
{
    Consumable,
    Equipment,
    Material,
    Quest,
    Misc
}

public class ItemDefinition
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public string Description { get; set; } = "";
    public ItemType Type { get; set; } = ItemType.Misc;
    public ItemRarity Rarity { get; set; } = ItemRarity.Common;
    public int MaxStackSize { get; set; } = 99;
    public bool IsUsable { get; set; }
    public bool IsTradable { get; set; } = true;
    public Dictionary<string, string> Properties { get; set; } = new();
}

public class ItemStack
{
    public ItemDefinition Item { get; set; } = null!;
    public int Count { get; set; }
    public string UniqueId { get; set; } = Guid.NewGuid().ToString("N")[..8];
}

public class ItemDatabase
{
    public Dictionary<string, ItemDefinition> Items { get; set; } = new();

    public void RegisterItem(ItemDefinition item)
    {
        Items[item.Id] = item;
    }

    public ItemDefinition? GetItem(string id)
    {
        return Items.TryGetValue(id, out var item) ? item : null;
    }

    public IEnumerable<ItemDefinition> GetAllItems()
    {
        return Items.Values;
    }

    public IEnumerable<ItemDefinition> GetByType(ItemType type)
    {
        return Items.Values.Where(i => i.Type == type);
    }

    public IEnumerable<ItemDefinition> GetByRarity(ItemRarity rarity)
    {
        return Items.Values.Where(i => i.Rarity == rarity);
    }
}
