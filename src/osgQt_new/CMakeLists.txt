cmake_minimum_required(VERSION 3.20)

project(osgqt_earth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(EarthProjectOptions)
include(EarthCompilerConfig)
include(EarthDependencyTargets)

function(earth_log_dependency_paths dependency_name)
    message(STATUS "---- ${dependency_name} paths ----")
    foreach(var IN LISTS ARGN)
        if(DEFINED ${var})
            set(value "${${var}}")
            if(value STREQUAL "")
                set(value "<empty>")
            endif()
            string(REPLACE ";" ", " pretty_value "${value}")
            message(STATUS "  ${var}: ${pretty_value}")
        else()
            message(STATUS "  ${var}: <undefined>")
        endif()
    endforeach()
endfunction()

earth_collect_feature_definitions(EARTH_FEATURE_DEFINITIONS)

set(OSGQT_OSG_COMPONENTS
    osg
    osgDB
    osgGA
    osgUtil
    osgViewer
    osgWidget
    osgText
    OpenThreads
)

set(OSG_COMPONENTS ${OSGQT_OSG_COMPONENTS})

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
find_package(OpenSceneGraph 3 REQUIRED COMPONENTS ${OSGQT_OSG_COMPONENTS})
find_package(osgEarth 3.7 REQUIRED)

earth_ensure_osg_components(${OSGQT_OSG_COMPONENTS})

earth_log_dependency_paths("Qt5"
    Qt5_DIR
    Qt5Core_DIR
    Qt5Gui_DIR
    Qt5Widgets_DIR
    Qt5OpenGL_DIR
)

earth_log_dependency_paths("OpenSceneGraph"
    OPENSCENEGRAPH_VERSION
    OPENSCENEGRAPH_INCLUDE_DIR
    OPENSCENEGRAPH_LIBRARIES
)

earth_log_dependency_paths("osgEarth"
    osgEarth_DIR
    OSGEARTH_FOUND
)

add_subdirectory(src)

if(EARTH_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(EARTH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
