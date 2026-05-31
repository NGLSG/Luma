namespace CutsceneSystem;

public enum TrackType { Camera, Entity, Audio, Dialogue, Wait, Custom }
public enum EaseType { Linear, EaseIn, EaseOut, EaseInOut }

public class CutsceneEvent
{
    public string Id { get; set; } = Guid.NewGuid().ToString("N")[..8];
    public float StartTime { get; set; }
    public float Duration { get; set; } = 1.0f;
    public TrackType Type { get; set; }
    public string TargetId { get; set; } = "";
    public Dictionary<string, string> Parameters { get; } = new();
    public EaseType Ease { get; set; } = EaseType.Linear;

    public float EndTime => StartTime + Duration;
}

public class CutsceneTrack
{
    public string Name { get; set; } = "Track";
    public TrackType Type { get; set; }
    public List<CutsceneEvent> Events { get; } = new();
    public bool IsMuted { get; set; }
}

public class Cutscene
{
    public string Id { get; set; } = Guid.NewGuid().ToString("N")[..8];
    public string Name { get; set; } = "New Cutscene";
    public float TotalDuration { get; set; } = 10.0f;
    public List<CutsceneTrack> Tracks { get; } = new();

    public CutsceneTrack AddTrack(string name, TrackType type)
    {
        var track = new CutsceneTrack { Name = name, Type = type };
        Tracks.Add(track);
        return track;
    }

    public void RemoveTrack(CutsceneTrack track) => Tracks.Remove(track);

    public CutsceneTrack? GetTrack(int index) =>
        index >= 0 && index < Tracks.Count ? Tracks[index] : null;

    public CutsceneEvent AddEvent(CutsceneTrack track, float startTime, float duration)
    {
        var evt = new CutsceneEvent
        {
            StartTime = startTime,
            Duration = duration,
            Type = track.Type
        };
        track.Events.Add(evt);
        return evt;
    }

    public void RemoveEvent(CutsceneTrack track, CutsceneEvent evt) => track.Events.Remove(evt);

    public List<CutsceneEvent> GetEventsAtTime(float time)
    {
        var result = new List<CutsceneEvent>();
        foreach (var track in Tracks)
        {
            if (track.IsMuted) continue;
            foreach (var evt in track.Events)
            {
                if (time >= evt.StartTime && time < evt.EndTime)
                    result.Add(evt);
            }
        }
        return result;
    }
}

public class CutsceneDatabase
{
    public List<Cutscene> Cutscenes { get; } = new();

    public Cutscene AddCutscene(string name)
    {
        var cs = new Cutscene { Name = name };
        Cutscenes.Add(cs);
        return cs;
    }

    public void RemoveCutscene(Cutscene cs) => Cutscenes.Remove(cs);

    public Cutscene? GetCutscene(int index) =>
        index >= 0 && index < Cutscenes.Count ? Cutscenes[index] : null;
}
