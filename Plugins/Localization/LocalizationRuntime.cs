namespace Localization;

public class LocalizationManager
{
    private static LocalizationManager? _instance;
    public static LocalizationManager Instance => _instance ??= new LocalizationManager();

    public string CurrentLanguage { get; private set; } = "zh-CN";
    public List<LocalizationTable> Tables { get; } = new();
    public event Action<string>? OnLanguageChanged;

    private LocalizationManager() { }

    public void AddTable(LocalizationTable table)
    {
        if (!Tables.Any(t => t.TableName == table.TableName))
            Tables.Add(table);
    }

    public string Get(string key)
    {
        return Get(key, CurrentLanguage);
    }

    public string Get(string key, string languageCode)
    {
        foreach (var table in Tables)
        {
            var entry = table.Entries.FirstOrDefault(e => e.Key == key);
            if (entry != null && entry.Translations.TryGetValue(languageCode, out var text)
                && !string.IsNullOrEmpty(text))
            {
                return text;
            }
        }
        return key;
    }

    public string GetFormatted(string key, params object[] args)
    {
        var template = Get(key);
        try { return string.Format(template, args); }
        catch { return template; }
    }

    public void SwitchLanguage(string languageCode)
    {
        if (CurrentLanguage == languageCode) return;
        CurrentLanguage = languageCode;
        OnLanguageChanged?.Invoke(languageCode);
    }

    public List<string> GetAvailableLanguages()
    {
        var codes = new HashSet<string>();
        foreach (var table in Tables)
            foreach (var lang in table.Languages)
                codes.Add(lang.Code);
        return codes.ToList();
    }

    public int GetTranslationCoverage(string languageCode)
    {
        int total = 0, translated = 0;
        foreach (var table in Tables)
        {
            foreach (var entry in table.Entries)
            {
                total++;
                if (entry.Translations.TryGetValue(languageCode, out var text)
                    && !string.IsNullOrEmpty(text))
                {
                    translated++;
                }
            }
        }
        return total == 0 ? 100 : (int)(translated * 100.0 / total);
    }
}
