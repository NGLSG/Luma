namespace Localization;

public class Language
{
    public string Code { get; set; }
    public string DisplayName { get; set; }
    public bool IsRTL { get; set; }

    public Language(string code, string displayName, bool isRTL = false)
    {
        Code = code;
        DisplayName = displayName;
        IsRTL = isRTL;
    }
}

public class LocalizedEntry
{
    public string Key { get; set; }
    public Dictionary<string, string> Translations { get; set; }

    public LocalizedEntry(string key)
    {
        Key = key;
        Translations = new Dictionary<string, string>();
    }
}

public class LocalizationTable
{
    public string TableName { get; set; }
    public List<Language> Languages { get; set; }
    public List<LocalizedEntry> Entries { get; set; }

    public LocalizationTable(string tableName)
    {
        TableName = tableName;
        Languages = new List<Language>();
        Entries = new List<LocalizedEntry>();
    }

    public void AddLanguage(string code, string displayName, bool isRTL = false)
    {
        if (Languages.Any(l => l.Code == code)) return;
        Languages.Add(new Language(code, displayName, isRTL));
    }

    public void RemoveLanguage(string code)
    {
        Languages.RemoveAll(l => l.Code == code);
        foreach (var entry in Entries)
            entry.Translations.Remove(code);
    }

    public void AddEntry(string key)
    {
        if (Entries.Any(e => e.Key == key)) return;
        Entries.Add(new LocalizedEntry(key));
    }

    public void RemoveEntry(string key)
    {
        Entries.RemoveAll(e => e.Key == key);
    }

    public void SetTranslation(string key, string languageCode, string text)
    {
        var entry = Entries.FirstOrDefault(e => e.Key == key);
        if (entry == null) return;
        entry.Translations[languageCode] = text;
    }

    public string GetTranslation(string key, string languageCode)
    {
        var entry = Entries.FirstOrDefault(e => e.Key == key);
        if (entry == null) return key;
        return entry.Translations.TryGetValue(languageCode, out var text) ? text : key;
    }

    public List<string> FindMissingTranslations(string languageCode)
    {
        var missing = new List<string>();
        foreach (var entry in Entries)
        {
            if (!entry.Translations.ContainsKey(languageCode) ||
                string.IsNullOrEmpty(entry.Translations[languageCode]))
            {
                missing.Add(entry.Key);
            }
        }
        return missing;
    }
}
