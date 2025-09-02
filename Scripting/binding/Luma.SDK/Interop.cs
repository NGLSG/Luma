using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Reflection;
using System.Text;
using YamlDotNet.Serialization;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Luma.SDK;





public static class Interop
{

    private static readonly Dictionary<IntPtr, Script> s_liveInstances = new();
    private static readonly object s_instanceLock = new object();
    private static IDeserializer? s_yamlDeserializer;

    private static IDeserializer YamlDeserializer =>
        s_yamlDeserializer ??= new DeserializerBuilder().Build();





    [UnmanagedCallersOnly]
    public static void InitializeDomain(IntPtr baseDirPtr)
    {
        try
        {
            string? baseDir = Marshal.PtrToStringUTF8(baseDirPtr);
            if (string.IsNullOrWhiteSpace(baseDir))
            {
                Debug.Log("[Domain] InitializeDomain skipped: baseDir is null or empty.");
                return;
            }

            Debug.Log($"[Domain] InitializeDomain: {baseDir}");
            DomainManager.Initialize(baseDir);
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] InitializeDomain failed: {e.Message}");
        }
    }





    [UnmanagedCallersOnly]
    public static void UnloadDomain()
    {
        try
        {

            lock (s_instanceLock)
            {
                if (s_liveInstances.Count > 0)
                {
                    Debug.Log($"[Domain] UnloadDomain: Cleaning up {s_liveInstances.Count} leaked instances.");
                    foreach (var kv in s_liveInstances.ToArray())
                    {
                        IntPtr handlePtr = kv.Key;
                        Script instance = kv.Value;


                        try
                        {
                            instance.OnDestroy();
                        }
                        catch (Exception ex)
                        {
                            Debug.Log($"[Domain] OnDestroy during unload failed: {ex.Message}");
                        }

                        try
                        {
                            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                            if (handle.IsAllocated) handle.Free();
                        }
                        catch
                        {

                        }

                        s_liveInstances.Remove(handlePtr);
                    }
                }
            }


            if (DomainManager.IsActive)
            {
                Debug.Log("[Domain] UnloadDomain: Unloading collectible ALC...");
                DomainManager.Unload();
            }


            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            Debug.Log("[Domain] UnloadDomain: Completed.");
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] UnloadDomain failed: {e.Message}");
        }
    }





    [UnmanagedCallersOnly]
    public static void Debug_ListAllTypesAndMethods(IntPtr assemblyPathPtr)
    {
        try
        {
            string? assemblyPath = Marshal.PtrToStringUTF8(assemblyPathPtr);
            if (string.IsNullOrEmpty(assemblyPath))
            {
                Debug.Log("[DEBUG] Assembly path is null or empty.");
                return;
            }

            Debug.Log($"\n--- [Reflection] Inspecting Assembly: {assemblyPath} ---");

            Assembly? assembly = DomainManager.LoadAssemblyFromPath(assemblyPath);
            assembly ??= Assembly.LoadFrom(assemblyPath);

            foreach (Type type in assembly.GetExportedTypes())
            {
                MethodInfo[] methods = type.GetMethods(BindingFlags.Public | BindingFlags.Instance |
                                                       BindingFlags.Static | BindingFlags.DeclaredOnly);

                if (methods.Length == 0)
                {
                    Debug.Log($"- Type: {type.AssemblyQualifiedName}");
                    continue;
                }

                foreach (MethodInfo method in methods)
                {
                    var sb = new StringBuilder();
                    sb.Append($"- Type: {type.AssemblyQualifiedName}, ");
                    sb.Append($"Method: {method.ReturnType.Name} {method.Name}(");

                    ParameterInfo[] parameters = method.GetParameters();
                    for (int i = 0; i < parameters.Length; i++)
                    {
                        sb.Append($"{parameters[i].ParameterType.Name} {parameters[i].Name}");
                        if (i < parameters.Length - 1)
                        {
                            sb.Append(", ");
                        }
                    }

                    sb.Append(")");
                    Debug.Log(sb.ToString());
                }
            }

            Debug.Log("--- [Reflection] Inspection Complete ---\n");
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] Debug_ListAllTypesAndMethods failed: {e.Message}");
        }
    }


    private static Type? ResolveType(string assemblyName, string typeName)
    {
        try
        {

            Type? type = DomainManager.ResolveType(assemblyName, typeName);
            if (type != null)
            {
                Debug.Log($"[TypeResolve] Found in ALC: {type.FullName} (asm: {assemblyName})");
                return type;
            }


            Assembly? assembly = AppDomain.CurrentDomain.GetAssemblies()
                .FirstOrDefault(a => a.GetName().Name == assemblyName);


            if (assembly == null)
            {
                Debug.Log(
                    $"[TypeResolve] Assembly '{assemblyName}' not found in current AppDomain. Attempting to load into default ALC...");
                assembly = Assembly.Load(assemblyName);
            }

            if (assembly != null)
            {
                Type? t = assembly.GetType(typeName, throwOnError: false, ignoreCase: false);
                if (t != null)
                {
                    Debug.Log($"[TypeResolve] Found in default ALC: {t.FullName} (asm: {assemblyName})");
                    return t;
                }
            }
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] ResolveType failed for '{typeName}' in '{assemblyName}': {e.Message}");
        }

        Debug.Log($"[ERROR] Failed to find type '{typeName}' in assembly '{assemblyName}'.");
        return null;
    }








    internal static Script? GetScriptInstance(IntPtr scenePtr, uint entityId, Type scriptType)
    {
        try
        {

            IntPtr scriptCompPtr = Native.Entity_GetComponent(scenePtr, entityId, "ScriptComponent");
            if (scriptCompPtr == IntPtr.Zero)
            {
                return null;
            }


            IntPtr handleAddress = Native.ScriptComponent_GetGCHandle(scriptCompPtr);
            if (handleAddress == IntPtr.Zero)
            {
                Debug.LogError(
                    $"[EXCEPTION] GetScriptInstance: Failed to get handle address pointer for entity {entityId}");
                return null;
            }


            IntPtr handleValue = Marshal.ReadIntPtr(handleAddress);
            if (handleValue == IntPtr.Zero)
            {
                Debug.LogWarning($"[GetScriptInstance] Handle for entity {entityId} is present but its value is zero.");
                return null;
            }


            lock (s_instanceLock)
            {
                if (s_liveInstances.TryGetValue(handleValue, out Script? instance))
                {

                    if (scriptType.IsAssignableFrom(instance.GetType()))
                    {
                        return instance;
                    }
                }
            }

            Debug.LogWarning(
                $"[GetScriptInstance] No script instance found for entity {entityId} with type {scriptType.FullName}. (Handle value: {handleValue})");
            return null;
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] GetScriptInstance failed: {e.Message}");
            return null;
        }
    }

    internal static bool HasScriptInstance(IntPtr scenePtr, uint entityId, Type scriptType)
    {
        var instance = GetScriptInstance(scenePtr, entityId, scriptType);
        return instance != null;
    }


    [UnmanagedCallersOnly]
    public static IntPtr CreateScriptInstance(IntPtr scenePtr, uint entityId, IntPtr typeNamePtr,
        IntPtr assemblyNamePtr)
    {
        try
        {
            Debug.Log($"[CreateScriptInstance] Starting creation for entity {entityId}");

            string? typeName = Marshal.PtrToStringUTF8(typeNamePtr);
            string? assemblyName = Marshal.PtrToStringUTF8(assemblyNamePtr);
            if (string.IsNullOrEmpty(typeName) || string.IsNullOrEmpty(assemblyName))
            {
                Debug.Log("CreateScriptInstance failed: typeName or assemblyName is null or empty.");
                return IntPtr.Zero;
            }


            if (scenePtr == IntPtr.Zero)
            {
                Debug.Log("CreateScriptInstance failed: scenePtr is null.");
                return IntPtr.Zero;
            }

            Debug.Log($"[CreateScriptInstance] Resolving type {typeName} from assembly {assemblyName}");


            Type? type = ResolveType(assemblyName, typeName);


            if (type == null || !typeof(Script).IsAssignableFrom(type))
            {
                Debug.Log(
                    $"CreateScriptInstance failed: Type '{typeName}' not found in assembly '{assemblyName}' or does not inherit from Luma.SDK.Script.");
                return IntPtr.Zero;
            }

            Debug.Log($"[CreateScriptInstance] Creating instance of type {type.FullName}");


            var inst = Activator.CreateInstance(type);
            if (inst is not Script instance)
            {
                Debug.Log(
                    $"CreateScriptInstance failed: Could not create instance or instance is not a Script for type '{typeName}'.");
                return IntPtr.Zero;
            }

            Debug.Log($"[CreateScriptInstance] Instance created successfully, setting Self property");


            try
            {
                var entity = new Entity(entityId, scenePtr);


                Debug.Log($"[CreateScriptInstance] Entity created: ID={entity.Id}, ScenePtr={entity.ScenePtr}");

                instance.Self = entity;
                Debug.Log($"[CreateScriptInstance] Successfully set Self for entity {entityId}");
            }
            catch (Exception ex)
            {
                Debug.Log(
                    $"CreateScriptInstance failed: Could not create Entity for entityId {entityId}: {ex.Message}");
                return IntPtr.Zero;
            }


            GCHandle handle = GCHandle.Alloc(instance);
            IntPtr handlePtr = GCHandle.ToIntPtr(handle);

            lock (s_instanceLock)
            {

                if (s_liveInstances.ContainsKey(handlePtr))
                {
                    Debug.LogWarning($"CreateScriptInstance: Handle {handlePtr} already exists, freeing new handle.");
                    handle.Free();
                    return IntPtr.Zero;
                }

                s_liveInstances[handlePtr] = instance;
            }

            Debug.Log(
                $"CreateScriptInstance: Successfully created instance for type '{typeName}', handle: {handlePtr}");
            return handlePtr;
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] CreateScriptInstance failed: {e.Message}\nStackTrace: {e.StackTrace}");
            return IntPtr.Zero;
        }
    }

    [UnmanagedCallersOnly]
    public static void OnCreate(IntPtr handleValue)
    {
        if (handleValue == IntPtr.Zero)
        {
            return;
        }

        try
        {

            if (s_liveInstances.TryGetValue(handleValue, out Script? instance) && instance != null)
            {
                instance.OnCreate();
            }
            else
            {
                Debug.LogWarning(
                    $"[Interop.OnCreate] Failed to find a live script instance for handle: {handleValue}. The OnCreate call was skipped.");
            }
        }
        catch (Exception e)
        {

            Debug.LogError(
                $"[Interop.OnCreate] An exception occurred within a script's OnCreate method. Handle: {handleValue}\nException: {e.Message}\nStackTrace: {e.StackTrace}");
        }
    }




    [UnmanagedCallersOnly]
    public static void DestroyScriptInstance(IntPtr handlePtr)
    {
        if (handlePtr == IntPtr.Zero)
        {
            Debug.Log("DestroyScriptInstance failed: handlePtr is null.");
            return;
        }

        Script? instance = null;
        lock (s_instanceLock)
        {
            if (!s_liveInstances.Remove(handlePtr, out instance))
            {
                Debug.Log($"DestroyScriptInstance: Instance not found for handle {handlePtr}.");
                return;
            }
        }

        try
        {
            if (instance != null)
            {
                instance.OnDestroy();
            }
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] DestroyScriptInstance.OnDestroy failed: {e.Message}");
        }

        try
        {
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            if (handle.IsAllocated)
            {
                handle.Free();
                Debug.Log($"DestroyScriptInstance: Successfully freed handle {handlePtr}");
            }
        }
        catch (Exception ex)
        {
            Debug.Log($"[EXCEPTION] DestroyScriptInstance.Free failed: {ex.Message}");
        }
    }




    [UnmanagedCallersOnly]
    public static void InvokeUpdate(IntPtr handlePtr, float deltaTime)
    {
        if (handlePtr == IntPtr.Zero) return;

        Script? instance = null;
        lock (s_instanceLock)
        {
            if (!s_liveInstances.TryGetValue(handlePtr, out instance))
            {
                return;
            }
        }

        try
        {
            instance?.OnUpdate(deltaTime);
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] InvokeUpdate failed: {e.Message}");
        }
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    [DynamicDependency(DynamicallyAccessedMemberTypes.All, typeof(Interop))]
    public static void DispatchCollisionEvent(IntPtr handlePtr, int contactType, uint otherEntityId)
    {
        if (handlePtr == IntPtr.Zero || !s_liveInstances.TryGetValue(handlePtr, out Script? instance))
        {
            return;
        }


        IntPtr scenePtr = Native.SceneManager_GetCurrentScene();
        if (scenePtr == IntPtr.Zero) return;


        Entity otherEntity = new Entity(otherEntityId, scenePtr);

        try
        {
            switch ((PhysicsContactType)contactType)
            {
                case PhysicsContactType.CollisionEnter:
                    instance.OnCollisionEnter(otherEntity);
                    break;
                case PhysicsContactType.CollisionExit:
                    instance.OnCollisionExit(otherEntity);
                    break;
                case PhysicsContactType.TriggerEnter:
                    instance.OnTriggerEnter(otherEntity);
                    break;
                case PhysicsContactType.TriggerExit:
                    instance.OnTriggerExit(otherEntity);
                    break;
                case PhysicsContactType.CollisionStay:
                    instance.OnCollisionStay(otherEntity);
                    break;
                case PhysicsContactType.TriggerStay:
                    instance.OnTriggerStay(otherEntity);
                    break;
                default:
                    break;
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"[C# EXCEPTION] in physics callback for {instance.GetType().Name}: {e.Message}");
        }
    }

    private enum PhysicsContactType
    {
        CollisionEnter,
        CollisionStay,
        CollisionExit,
        TriggerEnter,
        TriggerStay,
        TriggerExit
    }

    [UnmanagedCallersOnly]
    public static void OnEnable(IntPtr handlePtr)
    {
        if (handlePtr == IntPtr.Zero || !s_liveInstances.TryGetValue(handlePtr, out Script? instance))
        {
            return;
        }

        try
        {
            instance.OnEnable();
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] CallOnEnable failed: {e.Message}");
        }
    }

    [UnmanagedCallersOnly]
    public static void OnDisable(IntPtr handlePtr)
    {
        if (handlePtr == IntPtr.Zero || !s_liveInstances.TryGetValue(handlePtr, out Script? instance))
        {
            return;
        }

        try
        {
            instance.OnDisable();
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] CallOnDisable failed: {e.Message}");
        }
    }




    [UnmanagedCallersOnly]
    public static void SetExportedProperty(IntPtr handlePtr, IntPtr propNamePtr, IntPtr valueAsYamlPtr)
    {
        if (handlePtr == IntPtr.Zero || !s_liveInstances.TryGetValue(handlePtr, out Script? instance))
        {
            return;
        }

        try
        {
            string? propName = Marshal.PtrToStringUTF8(propNamePtr);
            string? valueAsYaml = Marshal.PtrToStringUTF8(valueAsYamlPtr);
            Debug.Log(
                $"SetExportedProperty: Set '{propName}' to '{valueAsYaml}' on instance of type '{instance.GetType().Name}'.");
            if (string.IsNullOrEmpty(propName) || valueAsYaml == null) return;

            FieldInfo? field = instance.GetType().GetField(propName);
            PropertyInfo? prop = instance.GetType().GetProperty(propName);

            MemberInfo? member = (MemberInfo?)field ?? prop;
            if (member == null || member.GetCustomAttribute<ExportAttribute>() == null)
            {
                return;
            }

            Type memberType = field?.FieldType ?? prop!.PropertyType;

            object? finalValue = null;


            if (typeof(Script).IsAssignableFrom(memberType))
            {


                string? guidStr = YamlDeserializer.Deserialize<string>(valueAsYaml);


                uint targetId = Native.Scene_FindGameObjectByGuid(instance.Self.ScenePtr, guidStr);
                if (targetId != 0)
                {
                    Entity targetEntity = new Entity(targetId, instance.Self.ScenePtr);

                    finalValue = targetEntity.GetScript(memberType);
                    if (finalValue == null)
                    {
                        Debug.LogWarning($"[SetExportedProperty] GetScript for GUID '{guidStr}' returned NULL. " +
                                         $"The property '{propName}' on '{instance.GetType().Name}' will be set to null.");
                    }
                    else
                    {
                        Debug.Log($"[SetExportedProperty] GetScript for GUID '{guidStr}' returned a valid instance. " +
                                  $"Proceeding to set property '{propName}'.");
                    }
                }
                else
                {
                    Debug.LogWarning(
                        $"SetExportedProperty: Entity with GUID '{guidStr}' not found in scene for property '{propName}'.");
                    return;
                }
            }
            else
            {

                finalValue = YamlDeserializer.Deserialize(valueAsYaml, memberType);
            }


            field?.SetValue(instance, finalValue);
            prop?.SetValue(instance, finalValue);
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] SetExportedProperty failed: {e.Message}");
        }
    }


    [UnmanagedCallersOnly]
    public static int SanityCheck(int a, int b)
    {
        Debug.Log($"[SanityCheck] Method called successfully with parameters: {a}, {b}.");
        return a + b;
    }

    [UnmanagedCallersOnly]
    public static void InvokeMethod(IntPtr handlePtr, IntPtr methodNamePtr, IntPtr argsAsYamlPtr)
    {
        if (handlePtr == IntPtr.Zero) return;

        Script? instance = null;
        lock (s_instanceLock)
        {
            if (!s_liveInstances.TryGetValue(handlePtr, out instance))
            {
                return;
            }
        }

        try
        {
            string? methodName = Marshal.PtrToStringUTF8(methodNamePtr);
            string? argsAsYaml = Marshal.PtrToStringUTF8(argsAsYamlPtr);
            if (string.IsNullOrEmpty(methodName)) return;


            MethodInfo? method = instance.GetType().GetMethod(methodName, BindingFlags.Public | BindingFlags.Instance);
            if (method == null) return;


            object?[]? parameters = null;
            var paramInfos = method.GetParameters();
            if (!string.IsNullOrEmpty(argsAsYaml) && paramInfos.Length > 0)
            {
                var deserializedArgs = YamlDeserializer.Deserialize<List<object>>(argsAsYaml);
                if (deserializedArgs != null && deserializedArgs.Count == paramInfos.Length)
                {
                    parameters = new object[paramInfos.Length];
                    for (int i = 0; i < paramInfos.Length; i++)
                    {
                        parameters[i] = Convert.ChangeType(deserializedArgs[i], paramInfos[i].ParameterType);
                    }
                }
            }


            method.Invoke(instance, parameters);
        }
        catch (Exception e)
        {
            Debug.Log($"[EXCEPTION] InvokeMethod failed: {e.Message}");
        }
    }
}