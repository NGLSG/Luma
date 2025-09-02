using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Luma.SDK;

public interface IJob
{
    void Execute();
}

public struct JobHandle
{
    internal IntPtr m_nativeHandle;

    internal JobHandle(IntPtr nativeHandle)
    {
        m_nativeHandle = nativeHandle;
    }

    public bool IsValid => m_nativeHandle != IntPtr.Zero;

    public void Complete()
    {
        if (IsValid)
        {
            JobSystem.Complete(this);
            m_nativeHandle = IntPtr.Zero;
        }
    }
}

public static class JobSystem
{
    private const string NATIVE_LIB_NAME = "LumaEngine";

    private delegate void ManagedJobCallback(IntPtr context);

    private static readonly ManagedJobCallback s_jobCallback = ExecuteManagedJob;
    private static readonly ManagedJobCallback s_freeGCHandleCallback = FreeGCHandle;

    public static int ThreadCount { get; }

    static JobSystem()
    {
        JobSystem_RegisterGCHandleFreeCallback(Marshal.GetFunctionPointerForDelegate(s_freeGCHandleCallback));
        ThreadCount = JobSystem_GetThreadCount();
    }

    public static JobHandle Schedule(Action job)
    {
        if (job == null)
        {
            throw new ArgumentNullException(nameof(job));
        }

        GCHandle handle = GCHandle.Alloc(job, GCHandleType.Normal);
        IntPtr context = GCHandle.ToIntPtr(handle);

        IntPtr callbackPtr = Marshal.GetFunctionPointerForDelegate(s_jobCallback);

        IntPtr nativeHandle = JobSystem_Schedule(callbackPtr, context);

        return new JobHandle(nativeHandle);
    }

    public static JobHandle Schedule(IJob job)
    {
        if (job == null)
        {
            throw new ArgumentNullException(nameof(job));
        }

        return Schedule(job.Execute);
    }

    public static void CompleteAll(List<JobHandle> handles)
    {
        if (handles == null || handles.Count == 0) return;

        var nativeHandles = new IntPtr[handles.Count];
        for (int i = 0; i < handles.Count; i++)
        {
            nativeHandles[i] = handles[i].m_nativeHandle;
        }

        JobSystem_CompleteAll(nativeHandles, nativeHandles.Length);
        handles.Clear();
    }

    internal static void Complete(JobHandle handle)
    {
        if (handle.IsValid)
        {
            JobSystem_Complete(handle.m_nativeHandle);
        }
    }

    private static void ExecuteManagedJob(IntPtr context)
    {
        try
        {
            GCHandle handle = GCHandle.FromIntPtr(context);
            if (handle.Target is Action job)
            {
                job.Invoke();
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"Exception in JobSystem worker thread: {e}");
        }
    }

    private static void FreeGCHandle(IntPtr context)
    {
        try
        {
            GCHandle handle = GCHandle.FromIntPtr(context);
            handle.Free();
        }
        catch (Exception e)
        {
            Debug.LogError($"Failed to free GCHandle: {e}");
        }
    }

    #region P/Invoke Declarations

    [DllImport(NATIVE_LIB_NAME, EntryPoint = "JobSystem_RegisterGCHandleFreeCallback",
        CallingConvention = CallingConvention.Cdecl)]
    private static extern void JobSystem_RegisterGCHandleFreeCallback(IntPtr freeCallback);

    [DllImport(NATIVE_LIB_NAME, EntryPoint = "JobSystem_Schedule", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr JobSystem_Schedule(IntPtr callback, IntPtr context);

    [DllImport(NATIVE_LIB_NAME, EntryPoint = "JobSystem_Complete", CallingConvention = CallingConvention.Cdecl)]
    private static extern void JobSystem_Complete(IntPtr handle);

    [DllImport(NATIVE_LIB_NAME, EntryPoint = "JobSystem_CompleteAll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void JobSystem_CompleteAll(IntPtr[] handles, int count);

    [DllImport(NATIVE_LIB_NAME, EntryPoint = "JobSystem_GetThreadCount",
        CallingConvention = CallingConvention.Cdecl)]
    private static extern int JobSystem_GetThreadCount();

    #endregion
}