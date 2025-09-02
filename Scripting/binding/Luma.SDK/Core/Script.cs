namespace Luma.SDK;




public abstract class Script : Component, IScript
{
    public virtual void OnCreate()
    {
    }

    public virtual void OnUpdate(float deltaTime)
    {
    }

    public virtual void OnDestroy()
    {
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