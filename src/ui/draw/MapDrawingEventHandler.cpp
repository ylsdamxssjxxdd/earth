#include "ui/draw/MapDrawingEventHandler.h"

#include "ui/draw/MapDrawingController.h"

#include <osgViewer/View>
#include <osgUtil/LineSegmentIntersector>

#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osgEarth/Terrain>

namespace earth::ui::draw {

MapDrawingEventHandler::MapDrawingEventHandler(MapDrawingController* controller, osgViewer::View* view)
    : m_controller(controller)
    , m_view(view) {
}

void MapDrawingEventHandler::setMapNode(osgEarth::MapNode* node) noexcept {
    m_mapNode = node;
}

bool MapDrawingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&) {
    if (m_controller == nullptr || !m_controller->interactionEnabled()) {
        return false;
    }
    if (m_view == nullptr || !m_mapNode.valid()) {
        return false;
    }

    MapGeoPoint geo{};
    auto sampleCurrent = [&]() -> bool {
        return samplePoint(ea, geo);
    };

    bool consumed = false;
    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::PUSH:
        if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && sampleCurrent()) {
            m_controller->pointerPress(geo);
            consumed = true;
        }
        break;
    case osgGA::GUIEventAdapter::DRAG:
        if ((ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) != 0u && sampleCurrent()) {
            m_controller->pointerDrag(geo);
            consumed = true;
        }
        break;
    case osgGA::GUIEventAdapter::RELEASE:
        if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && sampleCurrent()) {
            m_controller->pointerRelease(geo);
            consumed = true;
        }
        break;
    case osgGA::GUIEventAdapter::DOUBLECLICK:
        if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && sampleCurrent()) {
            m_controller->pointerDoubleClick(geo);
            consumed = true;
        }
        break;
    case osgGA::GUIEventAdapter::MOVE:
        if (sampleCurrent()) {
            m_controller->pointerMove(geo);
        }
        break;
    default:
        break;
    }

    return consumed;
}

bool MapDrawingEventHandler::samplePoint(const osgGA::GUIEventAdapter& ea, MapGeoPoint& outPoint) const {
    if (m_view == nullptr) {
        return false;
    }

    osgEarth::MapNode* mapNode = m_mapNode.get();
    if (mapNode == nullptr) {
        return false;
    }

    osgUtil::LineSegmentIntersector::Intersections hits;
    if (!m_view->computeIntersections(ea.getX(), ea.getY(), hits)) {
        return false;
    }
    if (hits.empty()) {
        return false;
    }

    const auto& intersection = *hits.begin();
    const osg::Vec3d world = intersection.getWorldIntersectPoint();

    osgEarth::GeoPoint geo;
    geo.fromWorld(mapNode->getMapSRS(), world);
    if (!geo.isValid()) {
        return false;
    }

    outPoint.longitudeDeg = geo.x();
    outPoint.latitudeDeg = geo.y();
    outPoint.altitudeMeters = geo.z();

    if (osgEarth::Terrain* terrain = mapNode->getTerrain(); terrain != nullptr) {
        double elevation = outPoint.altitudeMeters;
        if (terrain->getHeight(mapNode->getMapSRS(), outPoint.longitudeDeg, outPoint.latitudeDeg, &elevation)) {
            outPoint.altitudeMeters = elevation;
        }
    }

    return true;
}

} // namespace earth::ui::draw
