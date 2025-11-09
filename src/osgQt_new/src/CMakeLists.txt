set(OSGQT_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt/Export
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt/GraphicsWindowQt
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt/QFontImplementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt/QGraphicsViewAdapter
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt/QWidgetImage
)

set(OSGQT_SOURCES
    GraphicsWindowQt.cpp
    QFontImplementation.cpp
    QGraphicsViewAdapter.cpp
    QWidgetImage.cpp
    ReaderQFont.cpp
)

set(OSGQT_MOC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt/QGraphicsViewAdapter
)

set(OSG_COMPONENTS
    osg
    osgDB
    osgGA
    osgUtil
    osgViewer
    osgWidget
    osgText
    OpenThreads
)

set(OSGQT_QT_VERSION 5)
if(DEFINED Qt5Core_VERSION)
    string(REGEX MATCH "^([0-9]+)" _qt_major "${Qt5Core_VERSION}")
    if(_qt_major)
        set(OSGQT_QT_VERSION "${_qt_major}")
    endif()
endif()

set(OSGQT_VERSION_HEADER "${CMAKE_BINARY_DIR}/include/osgQt/Version")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include/osgQt")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.in" "${OSGQT_VERSION_HEADER}" @ONLY)

add_library(osgQt ${OSGQT_SOURCES} ${OSGQT_PUBLIC_HEADERS} ${OSGQT_MOC_HEADERS} "${OSGQT_VERSION_HEADER}")

add_library(osgQt::osgQt ALIAS osgQt)

target_include_directories(osgQt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${OPENSCENEGRAPH_INCLUDE_DIR}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

target_compile_definitions(osgQt PRIVATE OSGQT_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(osgQt PUBLIC OSGQT_LIBRARY_STATIC)
endif()

earth_apply_default_target_settings(osgQt)

set(OSGQT_OSG_LINK_LIBS "")
foreach(component IN LISTS OSG_COMPONENTS)
    if(TARGET OpenSceneGraph::${component})
        list(APPEND OSGQT_OSG_LINK_LIBS OpenSceneGraph::${component})
    endif()
endforeach()

if(OSGQT_OSG_LINK_LIBS)
    target_link_libraries(osgQt
        PUBLIC
            ${OSGQT_OSG_LINK_LIBS}
    )
else()
    target_link_libraries(osgQt
        PUBLIC
            ${OPENSCENEGRAPH_LIBRARIES}
    )
endif()

target_link_libraries(osgQt
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
)

set_target_properties(osgQt PROPERTIES
    OUTPUT_NAME osgQt
    DEBUG_POSTFIX d
)

install(TARGETS osgQt
    EXPORT osgQtTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/osgQt
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*"
)

install(FILES "${OSGQT_VERSION_HEADER}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/osgQt
)

install(EXPORT osgQtTargets
    NAMESPACE osgQt::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osgQt
)
