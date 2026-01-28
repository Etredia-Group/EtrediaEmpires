cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (EtrediaEmpires)

add_subdirectory(sdk/map)
add_subdirectory(sdk/assets)
add_subdirectory(utils)
add_subdirectory(p7lib)
add_subdirectory(server)
add_subdirectory(engine-api)
add_subdirectory(renderers/magnum-renderer)
add_subdirectory(engine)