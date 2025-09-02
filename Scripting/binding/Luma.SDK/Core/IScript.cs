namespace Luma.SDK;




public interface IScript
{
    
    
    
    void OnCreate();

    
    
    
    
    void OnUpdate(float deltaTime);

    
    
    
    void OnDestroy();

    
    
    
    
    void OnCollisionEnter(Entity other);

    
    
    
    
    void OnCollisionStay(Entity other);

    
    
    
    
    void OnCollisionExit(Entity other);

    
    
    
    
    void OnTriggerEnter(Entity other);

    
    
    
    
    void OnTriggerStay(Entity other);

    
    
    
    
    void OnTriggerExit(Entity other);


    
    
    
    void OnEnable();

    
    
    
    void OnDisable();
}