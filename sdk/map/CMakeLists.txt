cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (convert_map)

# find_package(glm CONFIG REQUIRED)

add_executable(${PROJECT_NAME} convert_map.cpp)

# target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
# target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/engine/include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

file(GLOB MAP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.bmp")

set(COMPILED_DIR "${CMAKE_SOURCE_DIR}/assets/map")

foreach(MAP ${MAP_FILES})
    get_filename_component(MAP_NAME ${MAP} NAME_WE)
    set(OUT_FILE "${COMPILED_DIR}/${MAP_NAME}.map")

    add_custom_command(
        OUTPUT ${OUT_FILE}
        COMMAND $<TARGET_FILE:${PROJECT_NAME}> ${MAP} ${OUT_FILE}
        DEPENDS ${MAP} ${PROJECT_NAME}
        COMMENT "Converting map ${MAP_NAME}"
        VERBATIM
    )

    list(APPEND COMPILED_BINARIES ${OUT_FILE})
endforeach()

add_custom_target(compile_maps ALL
    DEPENDS ${COMPILED_BINARIES}
)
