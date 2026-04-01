function(add_library_target TARGET_NAME)
    cmake_parse_arguments(
            ADDITIONAL
            ""
            ""
            "MODULES;MODULE_BASEDIRS"
            ""
            ${ARGN}
    )

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)


    project(${TARGET_NAME} LANGUAGES CXX)
    add_library(${TARGET_NAME})

    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        src/*.cpp
        src/*.c
    )

    file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
        include/*.hpp
        include/*.h
    )

    file(GLOB_RECURSE MODULES CONFIGURE_DEPENDS
        modules/*.cppm
        modules/*.ixx
    )

    list(APPEND MODULES ${ADDITIONAL_MODULES})
    set (MODULE_BASEDIRS ${PROJECT_SOURCE_DIR})
    list(APPEND MODULE_BASEDIRS ${ADDITIONAL_MODULE_BASEDIRS})

    target_sources(${TARGET_NAME} PUBLIC FILE_SET modules TYPE CXX_MODULES BASE_DIRS ${MODULE_BASEDIRS} FILES ${MODULES})
    target_sources(${TARGET_NAME} PUBLIC FILE_SET headers TYPE HEADERS BASE_DIRS ${PROJECT_SOURCE_DIR} FILES ${HEADERS})
    target_sources(${TARGET_NAME} PRIVATE ${SOURCES})

    # Clang modules
#    target_compile_options(${TARGET_NAME} PRIVATE
#        -fmodules
#        -fcxx-modules
#        -fbuiltin-module-map
#        -fimplicit-module-maps
#    )

    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

endfunction()
