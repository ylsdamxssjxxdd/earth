set(OSGQT_EXAMPLE_OSG_LIBS "")
foreach(component IN LISTS OSG_COMPONENTS)
    if(TARGET OpenSceneGraph::${component})
        list(APPEND OSGQT_EXAMPLE_OSG_LIBS OpenSceneGraph::${component})
    endif()
endforeach()

if(NOT OSGQT_EXAMPLE_OSG_LIBS)
    set(OSGQT_EXAMPLE_OSG_LIBS ${OPENSCENEGRAPH_LIBRARIES})
endif()

function(osgqt_configure_example target_name)
    target_link_libraries(${target_name}
        PRIVATE
            osgQt::osgQt
            ${OSGQT_EXAMPLE_OSG_LIBS}
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::OpenGL
    )
    target_include_directories(${target_name}
        PRIVATE
            ${OPENSCENEGRAPH_INCLUDE_DIR}
            ${OPENSCENEGRAPH_INCLUDE_DIRS}
    )
    earth_apply_default_target_settings(${target_name})
endfunction()

add_subdirectory(osgviewerQt)
add_subdirectory(osgqfont)
