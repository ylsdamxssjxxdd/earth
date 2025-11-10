set(EARTH_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# ---- osgQt integration (build from src/osgQt_new) ----
set(EARTH_OSGQT_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/osgQt_new/include")
file(MAKE_DIRECTORY "${EARTH_OSGQT_GENERATED_INCLUDE_DIR}/osgQt")

set(OSGQT_QT_VERSION 5)
if(DEFINED Qt5Core_VERSION)
    string(REGEX MATCH "^([0-9]+)" _earth_osgqt_qt_major "${Qt5Core_VERSION}")
    if(_earth_osgqt_qt_major)
        set(OSGQT_QT_VERSION "${_earth_osgqt_qt_major}")
    endif()
endif()

configure_file(
    "${EARTH_SOURCE_ROOT}/osgQt_new/src/Version.in"
    "${EARTH_OSGQT_GENERATED_INCLUDE_DIR}/osgQt/Version"
    @ONLY
)

set(EARTH_OSGQT_SOURCES
    osgQt_new/src/GraphicsWindowQt.cpp
    osgQt_new/src/QFontImplementation.cpp
    osgQt_new/src/QGraphicsViewAdapter.cpp
    osgQt_new/src/QWidgetImage.cpp
    osgQt_new/src/ReaderQFont.cpp
)

add_library(earth_osgqt STATIC ${EARTH_OSGQT_SOURCES})

set(_earth_osgqt_public_includes
    ${EARTH_SOURCE_ROOT}/osgQt_new/include
    ${EARTH_OSGQT_GENERATED_INCLUDE_DIR}
)
if(OPENSCENEGRAPH_INCLUDE_DIR)
    list(APPEND _earth_osgqt_public_includes ${OPENSCENEGRAPH_INCLUDE_DIR})
endif()
if(OPENSCENEGRAPH_INCLUDE_DIRS)
    list(APPEND _earth_osgqt_public_includes ${OPENSCENEGRAPH_INCLUDE_DIRS})
endif()

target_include_directories(earth_osgqt
    PUBLIC
        ${_earth_osgqt_public_includes}
)

target_compile_definitions(earth_osgqt
    PRIVATE
        OSGQT_LIBRARY
    PUBLIC
        OSGQT_LIBRARY_STATIC
)

set(_earth_osgqt_osg_components
    osg
    osgDB
    osgGA
    osgUtil
    osgViewer
    osgWidget
    osgText
    OpenThreads
)

set(_earth_osgqt_osg_targets "")
foreach(component IN LISTS _earth_osgqt_osg_components)
    if(TARGET OpenSceneGraph::${component})
        list(APPEND _earth_osgqt_osg_targets OpenSceneGraph::${component})
    endif()
endforeach()

if(_earth_osgqt_osg_targets)
    target_link_libraries(earth_osgqt
        PUBLIC
            ${_earth_osgqt_osg_targets}
    )
elseif(OPENSCENEGRAPH_LIBRARIES)
    target_link_libraries(earth_osgqt
        PUBLIC
            ${OPENSCENEGRAPH_LIBRARIES}
    )
endif()

target_link_libraries(earth_osgqt
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
)

earth_apply_target_defaults(earth_osgqt)

add_library(earth_core STATIC
    core/EnvironmentBootstrapper.cpp
    core/SimulationBootstrapper.cpp
)
target_include_directories(earth_core PUBLIC ${EARTH_SOURCE_ROOT})
target_link_libraries(earth_core
    PUBLIC
        Qt5::Core
        Qt5::Gui
        OpenCV::opencv_core
        OpenCV::opencv_imgproc
        osgEarth::osgEarth
        OpenSceneGraph::osg
        OpenSceneGraph::osgDB
        OpenSceneGraph::osgGA
        OpenSceneGraph::osgUtil
)
target_compile_definitions(earth_core PUBLIC ${EARTH_FEATURE_DEFINITIONS})
earth_apply_target_defaults(earth_core)

add_library(earth_ui STATIC
    ui/MainWindow.cpp
    ui/MainWindow.ui
    ui/SceneWidget.cpp
    ui/draw/MapDrawingController.cpp
    ui/draw/MapDrawingEventHandler.cpp
)
target_include_directories(earth_ui PUBLIC ${EARTH_SOURCE_ROOT})
target_link_libraries(earth_ui
    PUBLIC
        Qt5::Widgets
        Qt5::Gui
        earth_osgqt
        earth_core
        osgEarth::osgEarth
        OpenSceneGraph::osgViewer
        OpenSceneGraph::osgGA
        OpenSceneGraph::osgUtil
)
target_compile_definitions(earth_ui PUBLIC ${EARTH_FEATURE_DEFINITIONS})
earth_apply_target_defaults(earth_ui)

add_executable(airport_earth
    app/main.cpp
    ../resource/ui_resources.qrc
)
target_include_directories(airport_earth PRIVATE ${EARTH_SOURCE_ROOT})
target_link_libraries(airport_earth PRIVATE
    earth_ui
    Qt5::Widgets
    OpenCV::opencv_core
    OpenCV::opencv_imgproc
)
target_compile_definitions(airport_earth PRIVATE ${EARTH_FEATURE_DEFINITIONS})
earth_apply_target_defaults(airport_earth)
set_target_properties(airport_earth PROPERTIES OUTPUT_NAME airport-earth)
