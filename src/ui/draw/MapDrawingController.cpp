#include "ui/draw/MapDrawingController.h"

#include "ui/SceneWidget.h"
#include "ui/draw/MapDrawingEventHandler.h"

#include <algorithm>
#include <cmath>

#include <osg/Group>
#include <osg/Math>
#include <osgViewer/View>

#include <osgEarth/AltitudeSymbol>
#include <osgEarth/Color>
#include <osgEarth/Feature>
#include <osgEarth/FeatureNode>
#include <osgEarth/GeoData>
#include <osgEarth/GeoMath>
#include <osgEarth/Geometry>
#include <osgEarth/LineSymbol>
#include <osgEarth/MapNode>
#include <osgEarth/PointSymbol>
#include <osgEarth/PolygonSymbol>
#include <osgEarth/RenderSymbol>
#include <osgEarth/SpatialReference>
#include <osgEarth/Style>
#include <osgEarth/Units>

namespace {
constexpr double kMinSampleDistanceMeters = 1.0;
constexpr float kDefaultStrokeWidthPx = 4.0F;
constexpr float kPreviewAlphaScale = 0.65F;
constexpr float kMinStrokeThickness = 1.0F;
constexpr float kMaxStrokeThickness = 20.0F;
constexpr float kPointSizeFactor = 2.4F;
constexpr float kMinPointPixelSize = 8.0F;

earth::ui::draw::ColorRgba defaultStrokeColor() {
    return {0.97F, 0.58F, 0.20F, 1.0F};
}

osgEarth::Color toOsgColor(const earth::ui::draw::ColorRgba& color, float alphaScale = 1.0F) {
    const float alpha = std::clamp(color.a * alphaScale, 0.0F, 1.0F);
    return osgEarth::Color(color.r, color.g, color.b, alpha);
}
} // namespace

namespace earth::ui::draw {

MapDrawingController::MapDrawingController() {
    ensureRoot();
    m_wgs84 = osgEarth::SpatialReference::get("wgs84");
    m_strokeColor = defaultStrokeColor();
    m_strokeThickness = kDefaultStrokeWidthPx;
}

MapDrawingController::~MapDrawingController() {
    removeEventHandler();
    detachRoot();
    m_root = nullptr;
}

void MapDrawingController::attachSceneWidget(SceneWidget* widget) {
    if (m_sceneWidget == widget) {
        return;
    }

    removeEventHandler();
    m_sceneWidget = widget;
    m_view = nullptr;
    if (m_sceneWidget != nullptr) {
        m_view = m_sceneWidget->embeddedView();
    }

    installEventHandler();
}

void MapDrawingController::setMapNode(osgEarth::MapNode* node) {
    if (m_mapNode.get() == node) {
        return;
    }

    detachRoot();
    m_mapNode = node;
    attachRoot();

    if (m_eventHandler) {
        m_eventHandler->setMapNode(node);
    }
}

void MapDrawingController::setTool(DrawingTool tool) {
    if (m_activeTool == tool) {
        return;
    }

    m_activeTool = tool;
    m_interactionEnabled = (tool != DrawingTool::None);
    resetActivePrimitive();
}

void MapDrawingController::setStrokeColor(ColorRgba color) noexcept {
    m_strokeColor = color;
    rebuildPreview();
}

void MapDrawingController::setStrokeThickness(float thickness) noexcept {
    const float clamped = std::clamp(thickness, kMinStrokeThickness, kMaxStrokeThickness);
    if (std::abs(clamped - m_strokeThickness) < 0.01F) {
        return;
    }
    m_strokeThickness = clamped;
    rebuildPreview();
}

void MapDrawingController::clearDrawings() {
    if (m_root.valid()) {
        m_root->removeChildren(0, m_root->getNumChildren());
    }
    m_committedNodes.clear();
    resetActivePrimitive();
}

void MapDrawingController::pointerPress(const MapGeoPoint& point) {
    if (!m_interactionEnabled) {
        return;
    }

    switch (m_activeTool) {
    case DrawingTool::Point:
        addPointPrimitive(point);
        break;
    case DrawingTool::Polyline:
        appendPolylineVertex(point);
        break;
    case DrawingTool::Rectangle:
        beginRectangle(point);
        break;
    case DrawingTool::Freehand:
        m_activeVertices.clear();
        m_previewPoint.reset();
        m_activeVertices.push_back(point);
        m_freehandDrawing = true;
        rebuildPreview();
        break;
    default:
        break;
    }
}

void MapDrawingController::pointerDrag(const MapGeoPoint& point) {
    if (!m_interactionEnabled) {
        return;
    }

    if (m_freehandDrawing) {
        appendPolylineVertex(point, true);
    } else if (m_activeTool == DrawingTool::Rectangle && m_rectangleDragging) {
        updateRectanglePreview(point);
    } else if (m_activeTool == DrawingTool::Polyline && hasActiveVertices(1)) {
        m_previewPoint = point;
        rebuildPreview();
    }
}

void MapDrawingController::pointerRelease(const MapGeoPoint& point) {
    if (!m_interactionEnabled) {
        return;
    }

    if (m_freehandDrawing) {
        finalizePolyline();
        m_freehandDrawing = false;
    } else if (m_activeTool == DrawingTool::Rectangle && m_rectangleDragging) {
        finalizeRectangle(point);
    }
}

void MapDrawingController::pointerDoubleClick(const MapGeoPoint& point) {
    if (!m_interactionEnabled) {
        return;
    }

    if (m_freehandDrawing) {
        finalizePolyline();
        m_freehandDrawing = false;
    } else if (m_activeTool == DrawingTool::Polyline) {
        appendPolylineVertex(point);
        finalizePolyline();
    } else if (m_activeTool == DrawingTool::Rectangle && m_rectangleDragging) {
        finalizeRectangle(point, true);
    }
}

void MapDrawingController::pointerMove(const MapGeoPoint& point) {
    if (!m_interactionEnabled) {
        return;
    }

    if (m_freehandDrawing) {
        return;
    }

    if (m_activeTool == DrawingTool::Polyline && hasActiveVertices(1)) {
        m_previewPoint = point;
        rebuildPreview();
    } else if (m_activeTool == DrawingTool::Rectangle && m_rectangleDragging) {
        updateRectanglePreview(point);
    }
}

void MapDrawingController::ensureRoot() {
    if (!m_root.valid()) {
        m_root = new osg::Group();
        m_root->setName("MapDrawingRoot");
    }
}

void MapDrawingController::attachRoot() {
    if (!m_root.valid()) {
        return;
    }
    osgEarth::MapNode* node = m_mapNode.get();
    if (node == nullptr) {
        return;
    }
    if (!node->containsNode(m_root.get())) {
        node->addChild(m_root.get());
    }
}

void MapDrawingController::detachRoot() {
    osgEarth::MapNode* node = m_mapNode.get();
    if (node != nullptr && m_root.valid()) {
        node->removeChild(m_root.get());
    }
}

void MapDrawingController::installEventHandler() {
    if (!m_view.valid()) {
        return;
    }

    removeEventHandler();
    m_eventHandler = std::make_unique<MapDrawingEventHandler>(this, m_view.get());
    m_eventHandler->setMapNode(m_mapNode.get());
    m_view->addEventHandler(m_eventHandler.get());
    m_handlerView = m_view;
}

void MapDrawingController::removeEventHandler() {
    if (m_handlerView.valid() && m_eventHandler) {
        m_handlerView->removeEventHandler(m_eventHandler.get());
    }
    m_eventHandler.reset();
    m_handlerView = nullptr;
}

void MapDrawingController::rebuildPreview() {
    if (!m_root.valid()) {
        return;
    }

    if (m_previewNode.valid()) {
        m_root->removeChild(m_previewNode.get());
        m_previewNode = nullptr;
    }

    if (m_activeVertices.empty()) {
        return;
    }

    PrimitiveDefinition primitive;
    primitive.strokeColor = m_strokeColor;
    primitive.thicknessPixels = m_strokeThickness;

    std::optional<MapGeoPoint> previewPoint;

    if (m_activeTool == DrawingTool::Polyline) {
        primitive.type = PrimitiveType::Polyline;
        primitive.vertices = m_activeVertices;
        previewPoint = m_previewPoint;
    } else if (m_activeTool == DrawingTool::Rectangle && m_previewPoint.has_value()) {
        primitive.type = PrimitiveType::Polygon;
        primitive.vertices = buildRectangleVertices(m_activeVertices.front(), m_previewPoint.value());
        primitive.filled = true;
        primitive.strokeColor = m_strokeColor;
        primitive.fillOpacity = 0.28;
    } else {
        return;
    }

    osg::ref_ptr<osgEarth::FeatureNode> node = createNode(primitive, previewPoint, true);
    if (node.valid()) {
        m_previewNode = node;
        m_root->addChild(node.get());
    }
}

void MapDrawingController::resetActivePrimitive() {
    m_activeVertices.clear();
    m_previewPoint.reset();
    m_rectangleDragging = false;
    m_freehandDrawing = false;
    if (m_previewNode.valid() && m_root.valid()) {
        m_root->removeChild(m_previewNode.get());
        m_previewNode = nullptr;
    }
}

void MapDrawingController::addPointPrimitive(const MapGeoPoint& point) {
    PrimitiveDefinition primitive;
    primitive.type = PrimitiveType::Point;
    primitive.vertices = {point};
    primitive.strokeColor = m_strokeColor;
    primitive.thicknessPixels = std::max(static_cast<double>(m_strokeThickness), 1.0) * 1.6;
    commitPrimitive(primitive);
}

void MapDrawingController::appendPolylineVertex(const MapGeoPoint& point, bool forceSample) {
    const double minDistance =
        forceSample ? kMinSampleDistanceMeters * 0.25 : kMinSampleDistanceMeters;

    if (!m_activeVertices.empty()) {
        const MapGeoPoint& last = m_activeVertices.back();
        if (distanceMeters(last, point) < minDistance) {
            return;
        }
    }
    m_activeVertices.push_back(point);
    rebuildPreview();
}

void MapDrawingController::finalizePolyline() {
    if (!hasActiveVertices(2)) {
        resetActivePrimitive();
        return;
    }

    PrimitiveDefinition primitive;
    primitive.type = PrimitiveType::Polyline;
    primitive.vertices = m_activeVertices;
    primitive.strokeColor = m_strokeColor;
    primitive.thicknessPixels = m_strokeThickness;
    commitPrimitive(primitive);
    resetActivePrimitive();
}

void MapDrawingController::beginRectangle(const MapGeoPoint& anchor) {
    if (m_rectangleDragging) {
        return;
    }
    m_activeVertices.clear();
    m_activeVertices.push_back(anchor);
    m_previewPoint = anchor;
    m_rectangleDragging = true;
    rebuildPreview();
}

void MapDrawingController::updateRectanglePreview(const MapGeoPoint& current) {
    if (!m_rectangleDragging || m_activeVertices.empty()) {
        return;
    }
    m_previewPoint = current;
    rebuildPreview();
}

void MapDrawingController::finalizeRectangle(const MapGeoPoint& current, bool force) {
    if (!m_rectangleDragging || m_activeVertices.empty()) {
        resetActivePrimitive();
        return;
    }

    if (!force && distanceMeters(m_activeVertices.front(), current) < kMinSampleDistanceMeters) {
        resetActivePrimitive();
        return;
    }

    PrimitiveDefinition primitive;
    primitive.type = PrimitiveType::Polygon;
    primitive.vertices = buildRectangleVertices(m_activeVertices.front(), current);
    primitive.strokeColor = m_strokeColor;
    primitive.filled = true;
    primitive.fillOpacity = 0.35;
    primitive.thicknessPixels = m_strokeThickness;

    commitPrimitive(primitive);
    resetActivePrimitive();
}

void MapDrawingController::commitPrimitive(const PrimitiveDefinition& primitive, std::optional<MapGeoPoint> preview) {
    osg::ref_ptr<osgEarth::FeatureNode> node = createNode(primitive, preview, false);
    if (!node.valid() || !m_root.valid()) {
        return;
    }
    m_root->addChild(node.get());
    m_committedNodes.push_back(node);
}

osg::ref_ptr<osgEarth::FeatureNode> MapDrawingController::createNode(
    const PrimitiveDefinition& primitive,
    std::optional<MapGeoPoint> preview,
    bool previewNode) const {
    if (!m_wgs84.valid()) {
        return {};
    }

    osg::ref_ptr<osgEarth::Geometry> geometry;
    switch (primitive.type) {
    case PrimitiveType::Point:
        geometry = new osgEarth::PointSet();
        break;
    case PrimitiveType::Polyline:
        geometry = new osgEarth::LineString();
        break;
    case PrimitiveType::Polygon:
        geometry = new osgEarth::Polygon();
        break;
    }
    if (!geometry.valid()) {
        return {};
    }

    for (const MapGeoPoint& vertex : primitive.vertices) {
        geometry->push_back(vertex.longitudeDeg, vertex.latitudeDeg, vertex.altitudeMeters);
    }

    if (preview.has_value() && primitive.type != PrimitiveType::Point) {
        geometry->push_back(
            preview->longitudeDeg,
            preview->latitudeDeg,
            preview->altitudeMeters);
    }

    if (primitive.type == PrimitiveType::Polygon) {
        geometry->close();
    }

    osg::ref_ptr<osgEarth::Feature> feature = new osgEarth::Feature(geometry.get(), m_wgs84.get());
    feature->geoInterp() = osgEarth::GEOINTERP_GREAT_CIRCLE;

    osgEarth::Style style;
    osgEarth::AltitudeSymbol* altitude = style.getOrCreate<osgEarth::AltitudeSymbol>();
    altitude->clamping() = osgEarth::AltitudeSymbol::CLAMP_TO_TERRAIN;
    altitude->technique() = osgEarth::AltitudeSymbol::TECHNIQUE_DRAPE;
    altitude->binding() = osgEarth::AltitudeSymbol::BINDING_VERTEX;

    osgEarth::RenderSymbol* render = style.getOrCreate<osgEarth::RenderSymbol>();
    render->lighting() = false;
    render->depthTest() = true;
    render->depthOffset()->enabled() = true;
    render->depthOffset()->automatic() = true;
    render->transparent() = true;

    const float alphaScale = previewNode ? kPreviewAlphaScale : 1.0F;
    const osgEarth::Color strokeColor = toOsgColor(primitive.strokeColor, alphaScale);

    if (primitive.type == PrimitiveType::Point) {
        osgEarth::PointSymbol* point = style.getOrCreate<osgEarth::PointSymbol>();
        const float targetSize = static_cast<float>(primitive.thicknessPixels) * kPointSizeFactor;
        point->size() = std::max(targetSize, kMinPointPixelSize);
        point->fill()->color() = strokeColor;
        point->smooth() = true;
    } else {
        osgEarth::LineSymbol* line = style.getOrCreate<osgEarth::LineSymbol>();
        osgEarth::Stroke& stroke = line->stroke().mutable_value();
        stroke.color() = strokeColor;
        stroke.width() = osgEarth::Distance(primitive.thicknessPixels, osgEarth::Units::PIXELS);
        stroke.widthUnits() = osgEarth::Units::PIXELS;
        stroke.smooth() = true;

        if (primitive.type == PrimitiveType::Polygon && primitive.filled) {
            osgEarth::PolygonSymbol* polygon = style.getOrCreate<osgEarth::PolygonSymbol>();
            polygon->outline() = true;
            polygon->fill()->color() =
                toOsgColor(primitive.strokeColor, static_cast<float>(primitive.fillOpacity) * alphaScale);
        }
    }

    auto node = new osgEarth::FeatureNode(feature.get(), style);
    node->setName(previewNode ? "MapDrawingPreview" : "MapDrawingPrimitive");
    return node;
}

std::vector<MapGeoPoint> MapDrawingController::buildRectangleVertices(
    const MapGeoPoint& first,
    const MapGeoPoint& second) const {
    std::vector<MapGeoPoint> vertices;
    vertices.reserve(4);

    const double meanAltitude = 0.5 * (first.altitudeMeters + second.altitudeMeters);

    MapGeoPoint topLeft = first;
    topLeft.altitudeMeters = meanAltitude;

    MapGeoPoint topRight{second.longitudeDeg, first.latitudeDeg, meanAltitude};
    MapGeoPoint bottomRight = second;
    bottomRight.altitudeMeters = meanAltitude;
    MapGeoPoint bottomLeft{first.longitudeDeg, second.latitudeDeg, meanAltitude};

    vertices.push_back(topLeft);
    vertices.push_back(topRight);
    vertices.push_back(bottomRight);
    vertices.push_back(bottomLeft);
    return vertices;
}

bool MapDrawingController::hasActiveVertices(std::size_t minVertices) const {
    return m_activeVertices.size() >= minVertices;
}

double MapDrawingController::distanceMeters(const MapGeoPoint& a, const MapGeoPoint& b) {
    constexpr double kEarthRadius = 6378137.0;
    const double lat1 = osg::DegreesToRadians(a.latitudeDeg);
    const double lat2 = osg::DegreesToRadians(b.latitudeDeg);
    const double dLat = lat2 - lat1;
    const double dLon = osg::DegreesToRadians(b.longitudeDeg - a.longitudeDeg);

    const double sinHalfLat = std::sin(dLat / 2.0);
    const double sinHalfLon = std::sin(dLon / 2.0);
    double hav = sinHalfLat * sinHalfLat + std::cos(lat1) * std::cos(lat2) * sinHalfLon * sinHalfLon;
    hav = std::min(1.0, std::max(0.0, hav));
    return 2.0 * kEarthRadius * std::asin(std::sqrt(hav));
}

} // namespace earth::ui::draw
