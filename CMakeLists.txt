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

earth_collect_feature_definitions(EARTH_FEATURE_DEFINITIONS)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgproc)
find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgUtil osgViewer)
find_package(osgEarth REQUIRED)

add_subdirectory(src)

if(EARTH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
