cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (EEfrontend)


# List sources

set(HEADERS "Engine.hpp" "Renderer.hpp" "FileSystem.hpp" "compile_config.hpp" "AssetManager.hpp" "AssetData.hpp" "PipelineFactory.hpp")
set(SOURCES "main.cpp" "Engine.cpp" "Renderer.cpp" "FileSystem.cpp" "AssetManager.cpp" "AssetData.cpp" "PipelineFactory.cpp")

list(TRANSFORM HEADERS PREPEND "include/")
list(TRANSFORM SOURCES PREPEND "src/")

# Find dependencies

find_package(Boost REQUIRED COMPONENTS lockfree)
find_package(xxHash CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(RmlUi CONFIG REQUIRED)

# Link library

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC utils)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::lockfree)
target_link_libraries(${PROJECT_NAME} PRIVATE xxHash::xxhash)
target_link_libraries(${PROJECT_NAME} PRIVATE p7)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE RmlUi::RmlUi)

target_include_directories(${PROJECT_NAME} PUBLIC include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

# Compile options
if (APP_VERSION)
  string(REPLACE "." ";" APP_VERSION_LIST ${APP_VERSION})
  list(GET APP_VERSION_LIST 0 MAJOR_VERSION)
  list(GET APP_VERSION_LIST 1 MINOR_VERSION)
  list(GET APP_VERSION_LIST 2 PATCH_VERSION)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE APP_VERSION
    PRIVATE MAJOR_VERSION=${MAJOR_VERSION}
    PRIVATE MINOR_VERSION=${MINOR_VERSION}
    PRIVATE PATCH_VERSION=${PATCH_VERSION}
  )
endif()

if (VALIDATION_LAYER)
  target_compile_definitions(${PROJECT_NAME} PRIVATE VALIDATION_LAYER_ON)
endif()

# Set a symbolic link so executable can find the /assets folder
set(link_src "${CMAKE_SOURCE_DIR}/assets")
set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_custom_command( 
    TARGET ${PROJECT_NAME} 
    POST_BUILD COMMAND ${CMAKE_COMMAND} 
        -E create_symlink ${link_src} ${link_dst} 
    DEPENDS ${link_dst} 
    COMMENT "symbolic link ${link_src} => ${link_dst}" 
)