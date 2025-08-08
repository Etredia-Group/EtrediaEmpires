cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (EEfrontend)


# List sources

set(HEADERS "Renderer.hpp")
set(SOURCES "main.cpp" "Renderer.cpp")

list(TRANSFORM HEADERS PREPEND "include/")
list(TRANSFORM SOURCES PREPEND "src/")

# Find dependencies

find_package(Boost REQUIRED COMPONENTS lockfree)
find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(RmlUi CONFIG REQUIRED)

# Link library

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC utils)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::lockfree)
target_link_libraries(${PROJECT_NAME} PRIVATE p7)
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
  $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

set(RMLUI_BACKEND_PATH ${RmlUi_SOURCE_DIR}/Backends) # Link SDL Vulkan backend for RmlUi
add_library(rmlui_backend_SDL_VK INTERFACE)
target_sources(rmlui_backend_SDL_VK INTERFACE
	${RMLUI_BACKEND_PATH}/RmlUi_Platform_SDL.cpp
	${RMLUI_BACKEND_PATH}/RmlUi_Renderer_VK.cpp
)
target_include_directories(rmlui_backend_SDL_VK INTERFACE ${RMLUI_BACKEND_PATH})
target_compile_definitions(rmlui_backend_SDL_VK INTERFACE
	RMLUI_SDL_VERSION_MAJOR=2
	RMLUI_NUM_MSAA_SAMPLES=4
	RMLUI_VK_CUSTOM_LOADER=<vulkan/vulkan.h>
)
target_link_libraries(${PROJECT_NAME} PRIVATE RmlUi::RmlUi)

target_include_directories(${PROJECT_NAME} PUBLIC include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()