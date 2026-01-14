cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (compile_materials)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

add_executable(${PROJECT_NAME} compile_materials.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE xxHash::xxhash)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/engine/include/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

file(GLOB MATERIAL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.mat.json")

set(COMPILED_DIR "${CMAKE_SOURCE_DIR}/assets/materials")

foreach(MAT ${MATERIAL_FILES})
    get_filename_component(MAT_NAME ${MAT} NAME_WE)
    set(OUT_FILE "${COMPILED_DIR}/${MAT_NAME}.mat")

    add_custom_command(
        OUTPUT ${OUT_FILE}
        COMMAND ${PROJECT_NAME} ${MAT} ${OUT_FILE}
        DEPENDS ${MAT}
        COMMENT "Compiling material ${MAT_NAME}"
        VERBATIM
    )

    list(APPEND COMPILED_BINARIES ${OUT_FILE})
endforeach()

add_custom_target(compile_assets ALL
    DEPENDS ${COMPILED_BINARIES}
)
