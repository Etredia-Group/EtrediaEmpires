function(add_library_target TARGET_NAME)

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

    target_sources(${TARGET_NAME} PUBLIC FILE_SET modules TYPE CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR} FILES ${MODULES})
    target_sources(${TARGET_NAME} PUBLIC FILE_SET headers TYPE HEADERS BASE_DIRS ${PROJECT_SOURCE_DIR} FILES ${HEADERS})
    target_sources(${TARGET_NAME} PRIVATE ${SOURCES})

    # MSVC STD module linking
    # target_compile_options(${TARGET_NAME} PRIVATE
    #    /EHsc
    #    "SHELL:/reference \"std=$ENV{STD_MODULE_DIR}/std.ifc\""
    #   "SHELL:/reference \"std.compat=$ENV{STD_MODULE_DIR}/std.compat.ifc\""
    #)
    #target_link_libraries(${TARGET_NAME} PRIVATE "$ENV{STD_MODULE_DIR}/std.obj")
    # target_link_libraries(${TARGET_NAME} PRIVATE "$ENV{STD_MODULE_DIR}/std.compat.obj")
    #target_compile_options(${TARGET_NAME} PRIVATE
    #    /wd5050
    #)
    #target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23 INTERFACE cxx_std_23)

    # Clang modules
#    target_compile_options(${TARGET_NAME} PRIVATE
#        -fmodules
#        -fcxx-modules
#        -fbuiltin-module-map
#        -fimplicit-module-maps
#    )

    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

endfunction()
