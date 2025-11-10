#pragma once

#include "ui/draw/DrawingTypes.h"

#include <osg/observer_ptr>
#include <osg/ref_ptr>

#include <memory>
#include <optional>
#include <vector>

namespace osg {
class Group;
}

namespace osgViewer {
class View;
}

namespace osgEarth {
class FeatureNode;
class MapNode;
class SpatialReference;
}

namespace earth::ui {
class SceneWidget;
}

namespace earth::ui::draw {

class MapDrawingEventHandler;

/**
 * @brief 负责创建/管理贴地 FeatureNode 的绘制控制器，并与 SceneWidget 的输入事件交互。
 */
class MapDrawingController {
public:
    MapDrawingController();
    ~MapDrawingController();

    /**
     * @brief 绑定 Qt 场景窗口，使控制器能够向 osgViewer::View 注册事件处理器。
     */
    void attachSceneWidget(SceneWidget* widget);

    /**
     * @brief 更新当前使用的 MapNode，自动重新挂接绘制结果。
     */
    void setMapNode(osgEarth::MapNode* node);

    /**
     * @brief 激活指定绘制工具，None 表示关闭交互。
     */
    void setTool(DrawingTool tool);
    [[nodiscard]] DrawingTool tool() const noexcept { return m_activeTool; }
    [[nodiscard]] bool interactionEnabled() const noexcept { return m_interactionEnabled; }

    /**
     * @brief 统一设置当前绘制笔刷的颜色与粗细。
     */
    void setStrokeColor(ColorRgba color) noexcept;
    [[nodiscard]] ColorRgba strokeColor() const noexcept { return m_strokeColor; }
    void setStrokeThickness(float thickness) noexcept;
    [[nodiscard]] float strokeThickness() const noexcept { return m_strokeThickness; }

    /**
     * @brief 清空已提交的贴地图形，并复位预览。
     */
    void clearDrawings();

    // ---- 供事件处理器回调的接口 ----
    void pointerPress(const MapGeoPoint& point);
    void pointerDrag(const MapGeoPoint& point);
    void pointerRelease(const MapGeoPoint& point);
    void pointerDoubleClick(const MapGeoPoint& point);
    void pointerMove(const MapGeoPoint& point);

private:
    void ensureRoot();
    void attachRoot();
    void detachRoot();
    void installEventHandler();
    void removeEventHandler();
    void rebuildPreview();
    void resetActivePrimitive();

    void addPointPrimitive(const MapGeoPoint& point);
    void appendPolylineVertex(const MapGeoPoint& point);
    void finalizePolyline();
    void beginRectangle(const MapGeoPoint& anchor);
    void updateRectanglePreview(const MapGeoPoint& current);
    void finalizeRectangle(const MapGeoPoint& current, bool force = false);

    void commitPrimitive(const PrimitiveDefinition& primitive, std::optional<MapGeoPoint> preview = std::nullopt);
    [[nodiscard]] osg::ref_ptr<osgEarth::FeatureNode> createNode(
        const PrimitiveDefinition& primitive,
        std::optional<MapGeoPoint> preview,
        bool previewNode) const;

    std::vector<MapGeoPoint> buildRectangleVertices(const MapGeoPoint& first, const MapGeoPoint& second) const;
    [[nodiscard]] bool hasActiveVertices(std::size_t minVertices) const;
    [[nodiscard]] static double distanceMeters(const MapGeoPoint& a, const MapGeoPoint& b);

    SceneWidget* m_sceneWidget = nullptr;
    osg::observer_ptr<osgViewer::View> m_view;
    osg::observer_ptr<osgViewer::View> m_handlerView;
    osg::observer_ptr<osgEarth::MapNode> m_mapNode;
    osg::ref_ptr<osg::Group> m_root;
    osg::ref_ptr<osgEarth::FeatureNode> m_previewNode;
    std::unique_ptr<MapDrawingEventHandler> m_eventHandler;
    std::vector<osg::ref_ptr<osgEarth::FeatureNode>> m_committedNodes;

    std::vector<MapGeoPoint> m_activeVertices;
    std::optional<MapGeoPoint> m_previewPoint;
    DrawingTool m_activeTool = DrawingTool::None;
    bool m_interactionEnabled = false;
    bool m_rectangleDragging = false;

    osg::ref_ptr<const osgEarth::SpatialReference> m_wgs84;
    ColorRgba m_strokeColor { 0.97F, 0.58F, 0.20F, 1.0F };
    float m_strokeThickness = 4.0F;
};

} // namespace earth::ui::draw
