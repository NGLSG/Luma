# LumaEngine SDK CMake Config
# Usage:
#   find_package(LumaEngine REQUIRED)
#   target_link_libraries(myapp PRIVATE LumaEngine::LumaEngine)

include(CMakeFindDependencyMacro)

if(NOT TARGET LumaEngine::LumaEngine)
    include("${CMAKE_CURRENT_LIST_DIR}/LumaEngineTargets.cmake")
endif()
