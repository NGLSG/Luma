#ifndef PLATFORM_H
#define PLATFORM_H

// 平台相关导出宏定义，根据不同平台和编译选项设置符号可见性
#ifdef _WIN32

#ifdef LUMA_ENGINE_EXPORTS
#define LUMA_API __declspec(dllexport) ///< Windows 下导出符号

#else
#define LUMA_API __declspec(dllimport) ///< Windows 下导入符号
#endif
#else

#define LUMA_API __attribute__((visibility("default"))) ///< 非 Windows 平台下设置符号可见性
#endif

#include <cstddef>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __linux__
#include <unistd.h>
#include <sys/resource.h>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>

#endif

/**
 * @brief Platform 命名空间，封装跨平台的系统相关工具函数。
 */
namespace Platform
{
    /**
     * @brief 获取当前进程的内存使用量（单位：字节）。
     *        支持 Windows、Linux 和 macOS 平台，自动适配系统 API。
     * @return 当前进程占用的物理内存字节数，获取失败时返回 0。
     */
    inline size_t GetCurrentProcessMemoryUsage()
    {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            return pmc.WorkingSetSize;
        }
#elif __linux__
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)
        {
            return usage.ru_maxrss * 1024;
        }
#elif __APPLE__
        mach_task_basic_info_data_t info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) & info, &count) == KERN_SUCCESS)
        {
            return info.resident_size;
        }
#endif
        return 0;
    }
}
#endif