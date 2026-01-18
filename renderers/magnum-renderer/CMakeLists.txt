cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (magnum-renderer)

# List sources

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
    "include/*.hpp"
    "include/*.h"
)

# Find dependencies

if (WIN32)
  set(MAGNUM_CONTEXT WglContext)
elseif(UNIX)
  set(MAGNUM_CONTEXT GlxContext)
else()
  message(FATAL_ERROR "No supported context for system")
endif()

set(MAGNUM_WITH_GLFWAPPLICATION ON)
find_package(Magnum REQUIRED
    GL
    GlfwApplication
    Shaders
)

# Link library

add_library(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC engine-api)
target_link_libraries(${PROJECT_NAME} PUBLIC utils)

target_link_libraries(${PROJECT_NAME} PUBLIC p7)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Magnum::GL
    Magnum::GlfwApplication
    Magnum::Shaders
    Magnum::Magnum)

target_include_directories(${PROJECT_NAME} PUBLIC include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()