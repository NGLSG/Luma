using System;
using System.Collections;
using System.Runtime.InteropServices;
using Luma.SDK.Components;

namespace Luma.SDK;

public readonly struct Entity
{
    public readonly uint Id;
    public readonly IntPtr ScenePtr;
    private static readonly uint InvalidId = (uint)Int32.MaxValue;

    public string Name
    {
        get
        {
            IntPtr namePtr = Native.GameObject_GetName(ScenePtr, Id);
            if (namePtr == IntPtr.Zero)
            {
                return string.Empty;
            }

            return Marshal.PtrToStringAnsi(namePtr);
        }
        set => Native.GameObject_SetName(ScenePtr, Id, value);
    }

    public Entity Parent
    {
        get
        {
            if (!HasComponent<ParentComponent>())
            {
                return new Entity(InvalidId, ScenePtr);
            }

            var parentComp = GetComponent<ParentComponent>();
            return new Entity(parentComp.Parent, ScenePtr);
        }
        set
        {
            var parentComp = HasComponent<ParentComponent>() ? GetComponent<ParentComponent>() : new ParentComponent();
            parentComp.Parent = value.Id;
            parentComp.Enable = true;
            SetComponent(parentComp);
        }
    }

    public Entity[] Children
    {
        get
        {
            if (!HasComponent<ChildrenComponent>())
            {
                return [];
            }

            var childrenComp = GetComponent<ChildrenComponent>();
            int count = childrenComp.ChildrenCount;
            IntPtr ptr = childrenComp.Children;

            if (ptr == IntPtr.Zero || count <= 0)
            {
                return [];
            }

            uint[] childIds = new uint[count];
            Marshal.Copy(ptr, (int[])(object)childIds, 0, count);

            Entity[] children = new Entity[count];
            for (int i = 0; i < count; i++)
            {
                children[i] = new Entity(childIds[i], ScenePtr);
            }

            return children;
        }
    }

    internal Entity(uint entityId, IntPtr scene)
    {
        Id = entityId;
        ScenePtr = scene;
    }

    public bool ActiveSelf()
    {
        return Native.GameObject_IsActive(ScenePtr, Id);
    }

    public void SetActive(bool active)
    {
        Native.GameObject_SetActive(ScenePtr, Id, active);
    }

    public bool IsValid()
    {
        if (Id == InvalidId)
        {
            Debug.LogWarning("Entity is invalid because its ID is InvalidId.");
            return false;
        }

        if (ScenePtr == IntPtr.Zero)
        {
            Debug.LogWarning("Entity is invalid because its ScenePtr is IntPtr.Zero.");
            return false;
        }

        return true;
    }


    public T GetComponent<T>() where T : struct, IComponent
    {
        IntPtr componentPtr = Native.Entity_GetComponent(ScenePtr, Id, typeof(T).Name);
        if (componentPtr == IntPtr.Zero)
        {
            throw new Exception($"Native component '{typeof(T).Name}' not found on entity {Id}.");
        }

        return Marshal.PtrToStructure<T>(componentPtr);
    }


    public T? GetScript<T>() where T : Script
    {
        return Interop.GetScriptInstance(ScenePtr, Id, typeof(T)) as T;
    }

    public object? GetScript(Type scriptType)
    {
        return Interop.GetScriptInstance(ScenePtr, Id, scriptType);
    }


    public bool HasComponent<T>() where T : struct, IComponent
    {
        return Native.Entity_HasComponent(ScenePtr, Id, typeof(T).Name);
    }

    public T? AddComponent<T>() where T : struct, IComponent
    {
        IntPtr componentPtr = Native.Entity_AddComponent(ScenePtr, Id, typeof(T).Name);
        if (componentPtr == IntPtr.Zero)
        {
            throw new Exception($"Failed to add component '{typeof(T).Name}' to entity {Id}.");
        }

        return Marshal.PtrToStructure<T>(componentPtr);
    }


    public bool HasScript<T>() where T : Script
    {
        return Interop.HasScriptInstance(ScenePtr, Id, typeof(T));
    }

    public void RemoveComponent<T>() where T : struct, IComponent
    {
        Native.Entity_RemoveComponent(ScenePtr, Id, typeof(T).Name);
    }


    public unsafe void SetComponent<T>(in T component) where T : struct, IComponent
    {
        fixed (void* componentPtr = &component)
        {
            Native.Entity_SetComponent(ScenePtr, Id, typeof(T).Name, (IntPtr)componentPtr);
        }
    }

    public void InvokeEvent(string eventName)
    {
        Native.Event_Invoke_Void(ScenePtr, Id, eventName);
    }

    public void InvokeEvent<T1>(string eventName, T1 arg1)
    {
        if (arg1 is string strArg)
        {
            Native.Event_Invoke_String(ScenePtr, Id, eventName, strArg);
        }
        else if (arg1 is int intArg)
        {
            Native.Event_Invoke_Int(ScenePtr, Id, eventName, intArg);
        }
        else if (arg1 is float floatArg)
        {
            Native.Event_Invoke_Float(ScenePtr, Id, eventName, floatArg);
        }
        else if (arg1 is bool boolArg)
        {
            Native.Event_Invoke_Bool(ScenePtr, Id, eventName, boolArg);
        }
        else
        {
            throw new ArgumentException($"Unsupported argument type '{typeof(T1)}' for event '{eventName}'. " +
                                        "Supported types are: string, int, float, bool.");
        }
    }

    public void InvokeEvent(string eventName, params object[] args)
    {
        if (args == null || args.Length == 0)
        {
            Native.Event_Invoke_Void(ScenePtr, Id, eventName);
            return;
        }


        var serializer = new YamlDotNet.Serialization.SerializerBuilder().Build();
        string argsAsYaml = serializer.Serialize(args);

        Native.Event_InvokeWithArgs(ScenePtr, Id, eventName, argsAsYaml);
    }

    public List<Script> GetScripts()
    {
        var scripts = new List<Script>();

        try
        {
            
            int handleCount = 0;
            Native.ScriptComponent_GetAllGCHandlesCount(ScenePtr, Id, ref handleCount);

            if (handleCount <= 0)
            {
                return scripts;
            }

            
            IntPtr[] handles = new IntPtr[handleCount];
            unsafe
            {
                fixed (IntPtr* handlesPtr = handles)
                {
                    Native.ScriptComponent_GetAllGCHandles(ScenePtr, Id, (IntPtr)handlesPtr, handleCount);

                    
                    for (int i = 0; i < handleCount; i++)
                    {
                        IntPtr handlePtr = handles[i];
                        if (handlePtr != IntPtr.Zero)
                        {
                            Script? scriptInstance = Interop.GetScriptFromHandle(handlePtr);
                            if (scriptInstance != null)
                            {
                                scripts.Add(scriptInstance);
                            }
                        }
                    }
                }
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"获取脚本列表失败，实体ID: {Id}，错误: {e.Message}");
        }

        return scripts;
    }
}