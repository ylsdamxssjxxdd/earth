if(DEFINED EARTH_COMPILER_CONFIG_INCLUDED)
    return()
endif()
set(EARTH_COMPILER_CONFIG_INCLUDED TRUE)

include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
    add_compile_options(/permissive- /Zc:wchar_t /Zc:__cplusplus /MP)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

function(earth_apply_default_target_settings target_name)
    if(EARTH_FEATURE_DEFINITIONS)
        target_compile_definitions(${target_name} PRIVATE ${EARTH_FEATURE_DEFINITIONS})
    endif()

    if(MSVC)
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endfunction()
