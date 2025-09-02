#ifndef PCH_H
#define PCH_H
#include "Logger.h"
#include "Directory.h"
#include "Guid.h"
#include "LazySingleton.h"
#include "Utils.h"
#include "Platform.h"
#include <thread>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif
#endif
