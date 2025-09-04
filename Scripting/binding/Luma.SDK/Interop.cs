﻿using System.Diagnostics.CodeAnalysis;
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
                return type;
            }

            Assembly? assembly = AppDomain.CurrentDomain.GetAssemblies()
                .FirstOrDefault(a => a.GetName().Name == assemblyName);

            if (assembly == null)
            {
                assembly = Assembly.Load(assemblyName);
            }

            if (assembly != null)
            {
                Type? t = assembly.GetType(typeName, throwOnError: false, ignoreCase: false);
                if (t != null)
                {
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
            IntPtr scriptsCompPtr = Native.Entity_GetComponent(scenePtr, entityId, "ScriptsComponent");
            if (scriptsCompPtr == IntPtr.Zero)
            {
                return null;
            }

            IntPtr handleValue = Native.ScriptComponent_GetGCHandle(scriptsCompPtr, scriptType.Name);
            if (handleValue == IntPtr.Zero)
            {
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

            Type? type = ResolveType(assemblyName, typeName);

            if (type == null || !typeof(Script).IsAssignableFrom(type))
            {
                Debug.Log(
                    $"CreateScriptInstance failed: Type '{typeName}' not found in assembly '{assemblyName}' or does not inherit from Luma.SDK.Script.");
                return IntPtr.Zero;
            }

            var inst = Activator.CreateInstance(type);
            if (inst is not Script instance)
            {
                Debug.Log(
                    $"CreateScriptInstance failed: Could not create instance or instance is not a Script for type '{typeName}'.");
                return IntPtr.Zero;
            }

            try
            {
                var entity = new Entity(entityId, scenePtr);
                instance.Self = entity;
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
            return;
        }

        Script? instance = null;
        lock (s_instanceLock)
        {
            if (!s_liveInstances.Remove(handlePtr, out instance))
            {
                return;
            }
        }

        try
        {
            instance?.OnDestroy();
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
        Script? instance;
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
            if (string.IsNullOrEmpty(propName) || valueAsYaml == null) return;

            
            if (propName.StartsWith("__EventLink_"))
            {
                string eventName = propName.Substring("__EventLink_".Length);
                setupEventTargets(instance, eventName, valueAsYaml);
                return;
            }

            FieldInfo? field = instance.GetType().GetField(propName);
            PropertyInfo? prop = instance.GetType().GetProperty(propName);

            MemberInfo? member = (MemberInfo?)field ?? prop;
            if (member == null || member.GetCustomAttribute<ExportAttribute>() == null)
            {
                return;
            }

            Type memberType = field?.FieldType ?? prop!.PropertyType;
            object? finalValue;

            if (typeof(Script).IsAssignableFrom(memberType))
            {
                string? guidStr = YamlDeserializer.Deserialize<string>(valueAsYaml);
                uint targetId = Native.Scene_FindGameObjectByGuid(instance.Self.ScenePtr, guidStr);
                if (targetId != 0)
                {
                    Entity targetEntity = new Entity(targetId, instance.Self.ScenePtr);
                    finalValue = targetEntity.GetScript(memberType);
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

    private static void setupEventTargets(Script instance, string eventName, string targetsYaml)
    {
        try
        {
            
            FieldInfo? eventField = instance.GetType().GetField(eventName);
            if (eventField == null || !eventField.FieldType.Name.Contains("LumaEvent"))
            {
                Debug.LogWarning($"setupEventTargets: 未找到事件字段 '{eventName}' 或类型不匹配");
                return;
            }

            
            object? lumaEvent = eventField.GetValue(instance);
            if (lumaEvent == null)
            {
                Debug.LogWarning($"setupEventTargets: 事件字段 '{eventName}' 为空，尝试创建新实例");

                
                try
                {
                    lumaEvent = Activator.CreateInstance(eventField.FieldType);
                    eventField.SetValue(instance, lumaEvent);
                    Debug.Log($"setupEventTargets: 为事件字段 '{eventName}' 创建了新实例");
                }
                catch (Exception createEx)
                {
                    Debug.LogError($"setupEventTargets: 无法创建 LumaEvent 实例: {createEx.Message}");
                    return;
                }
            }

            
            var targets = YamlDeserializer.Deserialize<List<Dictionary<string, object>>>(targetsYaml);
            if (targets == null || targets.Count == 0)
            {
                Debug.LogWarning($"setupEventTargets: 事件 '{eventName}' 没有有效的目标");
                return;
            }

            
            Type lumaEventType = lumaEvent.GetType();
            MethodInfo? clearMethod = lumaEventType.GetMethod("Clear");
            clearMethod?.Invoke(lumaEvent, null);

            
            Type? delegateType = getDelegateTypeForLumaEvent(lumaEventType);
            if (delegateType == null)
            {
                Debug.LogError($"setupEventTargets: 无法确定事件 '{eventName}' 的委托类型");
                return;
            }

            MethodInfo? addMethod = lumaEventType.GetMethod("Add");
            if (addMethod == null)
            {
                Debug.LogError($"setupEventTargets: 事件类型 '{lumaEventType.Name}' 没有 Add 方法");
                return;
            }

            
            int successCount = 0;
            foreach (var target in targets)
            {
                if (!target.TryGetValue("entityGuid", out object? guidObj) ||
                    !target.TryGetValue("methodName", out object? methodObj))
                {
                    Debug.LogWarning("setupEventTargets: 目标缺少必要的字段 (entityGuid 或 methodName)");
                    continue;
                }

                string? guidStr = guidObj?.ToString();
                string? methodName = methodObj?.ToString();

                if (string.IsNullOrEmpty(guidStr) || string.IsNullOrEmpty(methodName))
                {
                    Debug.LogWarning("setupEventTargets: 目标的 entityGuid 或 methodName 为空");
                    continue;
                }

                uint targetEntityId = Native.Scene_FindGameObjectByGuid(instance.Self.ScenePtr, guidStr);
                if (targetEntityId == Int32.MaxValue)
                {
                    Debug.LogWarning($"setupEventTargets: 找不到 GUID 为 '{guidStr}' 的实体");
                    continue;
                }

                Entity targetEntity = new Entity(targetEntityId, instance.Self.ScenePtr);

                
                var targetScripts = targetEntity.GetScripts();
                bool methodFound = false;

                foreach (var targetScript in targetScripts)
                {
                    MethodInfo? targetMethod = findCompatibleMethod(targetScript.GetType(), methodName, delegateType);
                    if (targetMethod != null)
                    {
                        try
                        {
                            
                            Delegate? methodDelegate =
                                Delegate.CreateDelegate(delegateType, targetScript, targetMethod);
                            addMethod.Invoke(lumaEvent, new object[] { methodDelegate });
                            successCount++;
                            methodFound = true;
                            break;
                        }
                        catch (Exception delegateEx)
                        {
                            Debug.LogError($"setupEventTargets: 创建委托失败: {delegateEx.Message}");
                        }
                    }
                }

                if (!methodFound)
                {
                    Debug.LogWarning($"setupEventTargets: 在实体 '{guidStr}' 上找不到兼容的方法 '{methodName}'");
                }
            }

            Debug.Log($"setupEventTargets: 为事件 '{eventName}' 成功设置了 {successCount}/{targets.Count} 个目标");
        }
        catch (Exception e)
        {
            Debug.LogError($"setupEventTargets 失败: {e.Message}\n{e.StackTrace}");
        }
    }

    
    
    
    private static Type? getDelegateTypeForLumaEvent(Type lumaEventType)
    {
        if (lumaEventType == typeof(LumaEvent))
        {
            return typeof(Action);
        }

        if (lumaEventType.IsGenericType)
        {
            Type genericTypeDef = lumaEventType.GetGenericTypeDefinition();
            Type[] genericArgs = lumaEventType.GetGenericArguments();

            if (genericTypeDef == typeof(LumaEvent<>))
            {
                return typeof(Action<>).MakeGenericType(genericArgs);
            }

            if (genericTypeDef == typeof(LumaEvent<,>))
            {
                return typeof(Action<,>).MakeGenericType(genericArgs);
            }
        }

        return null;
    }

    
    
    
    private static MethodInfo? findCompatibleMethod(Type scriptType, string methodName, Type delegateType)
    {
        MethodInfo[] methods = scriptType.GetMethods(BindingFlags.Public | BindingFlags.Instance);

        foreach (MethodInfo method in methods)
        {
            if (method.Name != methodName)
                continue;

            try
            {
                
                Delegate.CreateDelegate(delegateType, null, method);
                return method;
            }
            catch
            {
                
                continue;
            }
        }

        return null;
    }

    public static Script? GetScriptFromHandle(IntPtr handlePtr)
    {
        if (handlePtr == IntPtr.Zero) return null;

        lock (s_instanceLock)
        {
            return s_liveInstances.TryGetValue(handlePtr, out Script? instance) ? instance : null;
        }
    }

    [UnmanagedCallersOnly]
    public static void InvokeMethod(IntPtr handlePtr, IntPtr methodNamePtr, IntPtr argsAsYamlPtr)
    {
        if (handlePtr == IntPtr.Zero) return;
        Script? instance;
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