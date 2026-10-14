cmake_minimum_required (VERSION 4.1)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_CXX_MODULE_STD ON)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (EtrediaEmpires LANGUAGES CXX)

add_subdirectory(sdk)

add_subdirectory(p7lib)
add_subdirectory(utils)

add_subdirectory(economy)

add_subdirectory(server/lib)
add_subdirectory(server/dedicated)
add_subdirectory(engine-api)

# add_subdirectory(renderers/magnum-renderer)
add_subdirectory(renderers/vulkan)

add_subdirectory(engine)