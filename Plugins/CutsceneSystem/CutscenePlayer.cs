namespace CutsceneSystem;

public class CutscenePlayer
{
    public Cutscene? CurrentCutscene { get; private set; }
    public float CurrentTime { get; private set; }
    public bool IsPlaying { get; private set; }
    public bool IsPaused { get; private set; }
    public float PlaybackSpeed { get; set; } = 1.0f;
    public List<string> Log { get; } = new();

    public float Progress => CurrentCutscene != null && CurrentCutscene.TotalDuration > 0
        ? CurrentTime / CurrentCutscene.TotalDuration
        : 0;

    public event Action<CutsceneEvent>? OnEventTriggered;
    public event Action? OnCutsceneEnd;

    private readonly HashSet<string> _triggeredEvents = new();

    public void Play(Cutscene cutscene)
    {
        CurrentCutscene = cutscene;
        CurrentTime = 0;
        IsPlaying = true;
        IsPaused = false;
        _triggeredEvents.Clear();
        Log.Clear();
        Log.Add($"[Play] {cutscene.Name} ({cutscene.TotalDuration:F1}s)");
    }

    public void Pause()
    {
        if (!IsPlaying) return;
        IsPaused = true;
        Log.Add($"[Pause] t={CurrentTime:F2}");
    }

    public void Resume()
    {
        if (!IsPlaying) return;
        IsPaused = false;
        Log.Add($"[Resume] t={CurrentTime:F2}");
    }

    public void Stop()
    {
        IsPlaying = false;
        IsPaused = false;
        Log.Add("[Stop]");
    }

    public void Seek(float time)
    {
        if (CurrentCutscene == null) return;
        CurrentTime = Math.Clamp(time, 0, CurrentCutscene.TotalDuration);
        _triggeredEvents.Clear();
        Log.Add($"[Seek] t={CurrentTime:F2}");
    }

    public void Update(float deltaTime)
    {
        if (!IsPlaying || IsPaused || CurrentCutscene == null) return;

        CurrentTime += deltaTime * PlaybackSpeed;

        var activeEvents = CurrentCutscene.GetEventsAtTime(CurrentTime);
        foreach (var evt in activeEvents)
        {
            if (_triggeredEvents.Add(evt.Id))
            {
                OnEventTriggered?.Invoke(evt);
                Log.Add($"[Event] {evt.Type} target={evt.TargetId} t={evt.StartTime:F2}");
            }
        }

        if (CurrentTime >= CurrentCutscene.TotalDuration)
        {
            CurrentTime = CurrentCutscene.TotalDuration;
            IsPlaying = false;
            OnCutsceneEnd?.Invoke();
            Log.Add("[End]");
        }
    }
}
