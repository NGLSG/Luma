namespace InventorySystem;

public class Inventory
{
    public string OwnerId { get; set; } = "";
    public int Capacity { get; set; }
    public List<ItemStack> Slots { get; set; }

    public event Action<ItemStack, int>? OnItemAdded;
    public event Action<ItemStack, int>? OnItemRemoved;

    public Inventory(int capacity = 20)
    {
        Capacity = capacity;
        Slots = new List<ItemStack>(capacity);
    }

    public bool AddItem(string itemId, int count, ItemDatabase db)
    {
        var def = db.GetItem(itemId);
        if (def == null) return false;

        foreach (var slot in Slots)
        {
            if (slot.Item.Id == itemId && slot.Count < slot.Item.MaxStackSize)
            {
                int canAdd = Math.Min(count, slot.Item.MaxStackSize - slot.Count);
                slot.Count += canAdd;
                count -= canAdd;
                OnItemAdded?.Invoke(slot, canAdd);
                if (count <= 0) return true;
            }
        }

        while (count > 0 && Slots.Count < Capacity)
        {
            int stackCount = Math.Min(count, def.MaxStackSize);
            var stack = new ItemStack { Item = def, Count = stackCount };
            Slots.Add(stack);
            count -= stackCount;
            OnItemAdded?.Invoke(stack, stackCount);
        }

        return count <= 0;
    }

    public bool RemoveItem(string itemId, int count)
    {
        if (!HasItem(itemId, count)) return false;

        for (int i = Slots.Count - 1; i >= 0 && count > 0; i--)
        {
            if (Slots[i].Item.Id != itemId) continue;
            int canRemove = Math.Min(count, Slots[i].Count);
            Slots[i].Count -= canRemove;
            count -= canRemove;
            OnItemRemoved?.Invoke(Slots[i], canRemove);
            if (Slots[i].Count <= 0)
                Slots.RemoveAt(i);
        }
        return true;
    }

    public ItemStack? GetItem(string itemId)
    {
        return Slots.FirstOrDefault(s => s.Item.Id == itemId);
    }

    public int GetItemCount(string itemId)
    {
        return Slots.Where(s => s.Item.Id == itemId).Sum(s => s.Count);
    }

    public bool HasItem(string itemId, int count = 1)
    {
        return GetItemCount(itemId) >= count;
    }

    public void MoveItem(int fromSlot, int toSlot)
    {
        if (fromSlot < 0 || fromSlot >= Slots.Count) return;
        if (toSlot < 0 || toSlot >= Slots.Count) return;
        (Slots[fromSlot], Slots[toSlot]) = (Slots[toSlot], Slots[fromSlot]);
    }

    public void SortByType()
    {
        Slots.Sort((a, b) => a.Item.Type.CompareTo(b.Item.Type));
    }

    public void SortByRarity()
    {
        Slots.Sort((a, b) => b.Item.Rarity.CompareTo(a.Item.Rarity));
    }

    public void SortByName()
    {
        Slots.Sort((a, b) => string.Compare(a.Item.Name, b.Item.Name, StringComparison.Ordinal));
    }

    public int GetUsedSlots() => Slots.Count;
    public int GetFreeSlots() => Capacity - Slots.Count;

    public void Clear()
    {
        Slots.Clear();
    }
}

public class EquipmentSlot
{
    public string SlotName { get; set; } = "";
    public ItemStack? EquippedItem { get; set; }
}

public class EquipmentManager
{
    public List<EquipmentSlot> Slots { get; set; } = new()
    {
        new EquipmentSlot { SlotName = "Weapon" },
        new EquipmentSlot { SlotName = "Armor" },
        new EquipmentSlot { SlotName = "Helmet" },
        new EquipmentSlot { SlotName = "Boots" },
        new EquipmentSlot { SlotName = "Accessory" }
    };

    public bool Equip(string slotName, ItemStack item)
    {
        var slot = Slots.FirstOrDefault(s => s.SlotName == slotName);
        if (slot == null) return false;
        slot.EquippedItem = item;
        return true;
    }

    public ItemStack? Unequip(string slotName)
    {
        var slot = Slots.FirstOrDefault(s => s.SlotName == slotName);
        if (slot == null) return null;
        var prev = slot.EquippedItem;
        slot.EquippedItem = null;
        return prev;
    }

    public ItemStack? GetEquipped(string slotName)
    {
        return Slots.FirstOrDefault(s => s.SlotName == slotName)?.EquippedItem;
    }
}
