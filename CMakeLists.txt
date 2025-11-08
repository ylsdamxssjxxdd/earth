cmake_minimum_required(VERSION 3.16)
project(airport_earth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(EarthProjectOptions)
include(EarthCompilerConfig)
include(EarthDependencyTargets)

function(earth_log_dependency_paths dependency_name)
    message(STATUS "---- ${dependency_name} 依赖路径 ----")
    foreach(var IN LISTS ARGN)
        if(DEFINED ${var})
            set(_value "${${var}}")
            if(_value STREQUAL "")
                set(_value "<空字符串>")
            endif()
            string(REPLACE ";" ", " _pretty_value "${_value}")
            message(STATUS "  ${var}: ${_pretty_value}")
        else()
            message(STATUS "  ${var}: <未设置>")
        endif()
    endforeach()
endfunction()

earth_collect_feature_definitions(EARTH_FEATURE_DEFINITIONS)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgUtil osgViewer)
find_package(osgEarth REQUIRED)

earth_ensure_opencv_components(core imgproc)
earth_ensure_osg_components(osg osgDB osgGA osgUtil osgViewer OpenThreads)

earth_log_dependency_paths("Qt5"
    Qt5_DIR
    Qt5Core_DIR
    Qt5Gui_DIR
    Qt5Widgets_DIR
)

earth_log_dependency_paths("OpenCV"
    OpenCV_DIR
)

earth_log_dependency_paths("OpenSceneGraph"
    OPENSCENEGRAPH_VERSION
    OSG_INCLUDE_DIR
    OSG_LIBRARY_DEBUG
    OSG_LIBRARY_RELEASE
    OSGDB_LIBRARY_DEBUG
    OSGDB_LIBRARY_RELEASE
    OSGGA_LIBRARY_DEBUG
    OSGGA_LIBRARY_RELEASE
    OSGUTIL_LIBRARY_DEBUG
    OSGUTIL_LIBRARY_RELEASE
    OSGVIEWER_LIBRARY_DEBUG
    OSGVIEWER_LIBRARY_RELEASE
    OPENTHREADS_LIBRARY_DEBUG
    OPENTHREADS_LIBRARY_RELEASE
)

earth_log_dependency_paths("osgEarth"
    osgEarth_DIR
)

add_subdirectory(src)

if(EARTH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
