message(STATUS "cmake config ${CMAKE_CURRENT_SOURCE_DIR}")

set(target magic_demo)

add_apps_libraries($<TARGET_FILE:${target}>)

add_library(${target} STATIC)

set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_unir_lib_dir})

target_include_directories(${target} PRIVATE
    ${SOURCE_TOP_DIR}
    ${SOURCE_SYS_DEBUG_DIR}
    ${SOURCE_SYS_OS_DIR}
    ${SOURCE_COMPONENTS_PUBLIC_INC})

target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${target} PRIVATE
    magic_demo.c
)

target_link_libraries_if(CONFIG_QOSA_LINK_SYSTEM_LIBRARIES_FUNC THEN ${target} system_os)