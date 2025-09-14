using System.Runtime.InteropServices;
using Luma.SDK.Generation;

namespace Luma.SDK.Components;

[StructLayout(LayoutKind.Sequential)]
public struct SpriteComponent : IComponent
{
    [MarshalAs(UnmanagedType.U1)] public bool Enable;
    public AssetHandle textureHandle;
    public AssetHandle materialHandle;
    public RectF sourceRect;
    public Color color;
    public int zIndex;
}




public class Sprite : LogicComponent<SpriteComponent>
{
    
    
    
    private const string ComponentName = "SpriteComponent";

    
    
    
    
    public Sprite(Entity entity) : base(entity)
    {
    }

    
    
    
    public Color Color
    {
        
        get => _component.color;
        
        
        set
        {
            
            _component.color = value;
            
            
            
            Entity.SetComponentProperty(ComponentName, "color", in value);
        }
    }

    
    
    
    public int ZIndex
    {
        get => _component.zIndex;
        set
        {
            _component.zIndex = value;
            Entity.SetComponentProperty(ComponentName, "zIndex", in value);
        }
    }

    
    
    
    public RectF SourceRect
    {
        get => _component.sourceRect;
        set
        {
            _component.sourceRect = value;
            Entity.SetComponentProperty(ComponentName, "sourceRect", in value);
        }
    }

    
    
    
    public AssetHandle TextureHandle
    {
        get => _component.textureHandle;
        set
        {
            _component.textureHandle = value;
            Entity.SetComponentProperty(ComponentName, "textureHandle", in value);
        }
    }

    
    
    
    public AssetHandle MaterialHandle
    {
        get => _component.materialHandle;
        set
        {
            _component.materialHandle = value;
            Entity.SetComponentProperty(ComponentName, "materialHandle", in value);
        }
    }
}