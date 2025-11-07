// Unified managed runtime host selector
#pragma once

#if defined(__ANDROID__) || defined(LUMA_PLATFORM_ANDROID)
#include "MonoHost.h"
using ManagedHost = MonoHost;
#else
#include "CoreCLRHost.h"
using ManagedHost = CoreCLRHost;
#endif
