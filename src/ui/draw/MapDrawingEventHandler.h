#pragma once

#include "ui/draw/DrawingTypes.h"

#include <osg/observer_ptr>
#include <osgGA/GUIEventHandler>

namespace osgViewer {
class View;
}

namespace osgEarth {
class MapNode;
}

namespace earth::ui::draw {

class MapDrawingController;

/**
 * @brief 将 osgGA 鼠标事件转换为贴地采样点并交由 MapDrawingController 处理。
 *
 * 事件处理器只在绘制工具被激活时才会消耗鼠标事件，其余时间委托给 osgEarth 的默认操纵器，
 * 因此不会影响原有漫游体验。
 */
class MapDrawingEventHandler final : public osgGA::GUIEventHandler {
public:
    MapDrawingEventHandler(MapDrawingController* controller, osgViewer::View* view);

    /**
     * @brief 更新用于求交测试的 MapNode。
     */
    void setMapNode(osgEarth::MapNode* node) noexcept;

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

private:
    [[nodiscard]] bool samplePoint(const osgGA::GUIEventAdapter& ea, MapGeoPoint& outPoint) const;

    MapDrawingController* m_controller = nullptr;
    osgViewer::View* m_view = nullptr;
    osg::observer_ptr<osgEarth::MapNode> m_mapNode;
};

} // namespace earth::ui::draw
