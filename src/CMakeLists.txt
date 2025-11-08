set(EARTH_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_library(earth_core STATIC
    core/SimulationBootstrapper.cpp
)
target_include_directories(earth_core PUBLIC ${EARTH_SOURCE_ROOT})
target_link_libraries(earth_core
    PUBLIC
        Qt5::Core
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
    ../resource/ui_resources.qrc
)
target_include_directories(earth_ui PUBLIC ${EARTH_SOURCE_ROOT})
target_link_libraries(earth_ui
    PUBLIC
        Qt5::Widgets
        Qt5::Gui
        earth_core
        OpenSceneGraph::osgViewer
        OpenSceneGraph::osgGA
)
target_compile_definitions(earth_ui PUBLIC ${EARTH_FEATURE_DEFINITIONS})
earth_apply_target_defaults(earth_ui)

add_executable(airport_earth
    app/main.cpp
)
target_include_directories(airport_earth PRIVATE ${EARTH_SOURCE_ROOT})
target_link_libraries(airport_earth PRIVATE earth_ui Qt5::Widgets)
target_compile_definitions(airport_earth PRIVATE ${EARTH_FEATURE_DEFINITIONS})
earth_apply_target_defaults(airport_earth)
set_target_properties(airport_earth PROPERTIES OUTPUT_NAME airport-earth)
