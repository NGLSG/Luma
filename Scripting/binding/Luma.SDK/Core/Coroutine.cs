using System;
using System.Collections;
using System.Collections.Generic;

namespace Luma.SDK;

public class WaitForSeconds
{
    public float Duration { get; }
    public WaitForSeconds(float seconds) { Duration = seconds; }
}

public class WaitForEndOfFrame { }

public class WaitUntil
{
    public Func<bool> Predicate { get; }
    public WaitUntil(Func<bool> predicate) { Predicate = predicate; }
}

public class Coroutine
{
    internal int Id { get; }
    internal Coroutine(int id) { Id = id; }
}

public class CoroutineManager
{
    private class CoroutineInstance
    {
        public int Id;
        public Stack<IEnumerator> Stack = new();
        public float WaitTimer;
        public bool WaitingForEndOfFrame;
        public Func<bool> WaitPredicate;
    }

    private readonly List<CoroutineInstance> _running = new();
    private readonly List<CoroutineInstance> _toRemove = new();
    private int _nextId;

    public Coroutine StartCoroutine(IEnumerator routine)
    {
        var inst = new CoroutineInstance { Id = _nextId++ };
        inst.Stack.Push(routine);
        _running.Add(inst);
        return new Coroutine(inst.Id);
    }

    public void StopCoroutine(Coroutine coroutine)
    {
        if (coroutine == null) return;
        _running.RemoveAll(c => c.Id == coroutine.Id);
    }

    public void StopAllCoroutines()
    {
        _running.Clear();
    }

    internal void Update(float deltaTime)
    {
        _toRemove.Clear();

        for (int i = 0; i < _running.Count; i++)
        {
            var inst = _running[i];

            if (inst.WaitingForEndOfFrame)
            {
                inst.WaitingForEndOfFrame = false;
            }

            if (inst.WaitTimer > 0f)
            {
                inst.WaitTimer -= deltaTime;
                if (inst.WaitTimer > 0f)
                    continue;
            }

            if (inst.WaitPredicate != null)
            {
                if (!inst.WaitPredicate())
                    continue;
                inst.WaitPredicate = null;
            }

            if (!Advance(inst))
                _toRemove.Add(inst);
        }

        foreach (var r in _toRemove)
            _running.Remove(r);
    }

    private bool Advance(CoroutineInstance inst)
    {
        while (inst.Stack.Count > 0)
        {
            var current = inst.Stack.Peek();
            bool moved;
            try { moved = current.MoveNext(); }
            catch { inst.Stack.Clear(); return false; }

            if (!moved)
            {
                inst.Stack.Pop();
                continue;
            }

            var yielded = current.Current;

            if (yielded == null)
                return true;

            if (yielded is WaitForSeconds wfs)
            {
                inst.WaitTimer = wfs.Duration;
                return true;
            }

            if (yielded is WaitForEndOfFrame)
            {
                inst.WaitingForEndOfFrame = true;
                return true;
            }

            if (yielded is WaitUntil wu)
            {
                inst.WaitPredicate = wu.Predicate;
                return true;
            }

            if (yielded is IEnumerator nested)
            {
                inst.Stack.Push(nested);
                continue;
            }

            if (yielded is Coroutine sub)
            {
                var subInst = _running.Find(c => c.Id == sub.Id);
                if (subInst != null)
                {
                    inst.WaitPredicate = () => !_running.Exists(c => c.Id == sub.Id);
                }
                return true;
            }

            return true;
        }

        return false;
    }
}
