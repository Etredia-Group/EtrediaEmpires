cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (compile_assets)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

add_executable(${PROJECT_NAME} compile_assets.cpp define/regions.def)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE xxHash::xxhash)
target_link_libraries(${PROJECT_NAME} PUBLIC engine-api)
target_link_libraries(${PROJECT_NAME} PUBLIC server)

target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()
