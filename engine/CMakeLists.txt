include(${CMAKE_SOURCE_DIR}/cmake/executable.cmake)

set(EXE engine)

# Find dependencies

find_package(Boost REQUIRED COMPONENTS lockfree)
find_package(xxHash CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Link library

add_executable_target(${EXE})

target_link_libraries(${EXE} PUBLIC utils)
target_link_libraries(${EXE} PUBLIC engine-api)
target_link_libraries(${EXE} PUBLIC magnum-renderer)

target_link_libraries(${EXE} PRIVATE Boost::lockfree)
target_link_libraries(${EXE} PRIVATE xxHash::xxhash)
target_link_libraries(${EXE} PRIVATE p7)
target_link_libraries(${EXE} PRIVATE glfw)
target_link_libraries(${EXE} PRIVATE glm::glm)

# Compile options
if (APP_VERSION)
    string(REPLACE "." ";" APP_VERSION_LIST ${APP_VERSION})
    list(GET APP_VERSION_LIST 0 MAJOR_VERSION)
    list(GET APP_VERSION_LIST 1 MINOR_VERSION)
    list(GET APP_VERSION_LIST 2 PATCH_VERSION)
    target_compile_definitions(${EXE}
            PRIVATE APP_VERSION
            PRIVATE MAJOR_VERSION=${MAJOR_VERSION}
            PRIVATE MINOR_VERSION=${MINOR_VERSION}
            PRIVATE PATCH_VERSION=${PATCH_VERSION}
    )
endif()

# Set a symbolic link so executable can find the /assets folder
set(link_src "${CMAKE_SOURCE_DIR}/assets")
set(link_dst "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_custom_command(
        TARGET ${EXE}
        POST_BUILD COMMAND ${CMAKE_COMMAND}
        -E create_symlink ${link_src} ${link_dst}
        COMMENT "symbolic link ${link_src} => ${link_dst}"
)