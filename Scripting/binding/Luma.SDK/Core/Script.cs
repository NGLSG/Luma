using System.Collections;

namespace Luma.SDK;

public abstract class Script : Component, IScript
{
    private readonly CoroutineManager _coroutineManager = new();

    public virtual void OnCreate()
    {
    }

    public virtual void OnUpdate(float deltaTime)
    {
        _coroutineManager.Update(deltaTime);
    }

    public virtual void OnDestroy()
    {
        _coroutineManager.StopAllCoroutines();
    }

    protected Coroutine StartCoroutine(IEnumerator routine)
    {
        return _coroutineManager.StartCoroutine(routine);
    }

    protected void StopCoroutine(Coroutine coroutine)
    {
        _coroutineManager.StopCoroutine(coroutine);
    }

    protected void StopAllCoroutines()
    {
        _coroutineManager.StopAllCoroutines();
    }

    public virtual void OnCollisionEnter(Entity other)
    {
    }

    public virtual void OnCollisionStay(Entity other)
    {
    }

    public virtual void OnCollisionExit(Entity other)
    {
    }

    public virtual void OnTriggerEnter(Entity other)
    {
    }

    public virtual void OnTriggerStay(Entity other)
    {
    }

    public virtual void OnTriggerExit(Entity other)
    {
    }

    public virtual void OnEnable()
    {
    }

    public virtual void OnDisable()
    {
    }
}