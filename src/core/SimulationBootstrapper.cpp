#include "core/SimulationBootstrapper.h"

#include <algorithm>
#include <osg/Geode>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Vec3>
#include <osgEarth/MapNode>
#include <opencv2/imgproc.hpp>

namespace earth::core {

SimulationBootstrapper::SimulationBootstrapper()
    : m_root(new osg::Group())
    , m_map(new osgEarth::Map()) {
}

void SimulationBootstrapper::initialize() {
    buildSceneGraph();
    m_cachedRunwayMask = buildRunwayMask(128);
}

osg::Group* SimulationBootstrapper::sceneRoot() const {
    return m_root.get();
}

cv::Mat SimulationBootstrapper::runwayMask() const {
    return m_cachedRunwayMask;
}

void SimulationBootstrapper::buildSceneGraph() {
    if (!m_root.valid()) {
        return;
    }

    m_root->removeChildren(0, m_root->getNumChildren());

    osg::ref_ptr<osgEarth::MapNode> mapNode = new osgEarth::MapNode(m_map.get());
    mapNode->setName("AirportMapNode");
    m_root->addChild(mapNode.get());

    osg::ref_ptr<osg::Geode> runwayGeode = new osg::Geode();
    osg::ref_ptr<osg::ShapeDrawable> runwayGeometry =
        new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 0.0f), 1000.0f, 60.0f, 2.0f));
    runwayGeometry->setName("ProceduralRunway");
    runwayGeode->addDrawable(runwayGeometry.get());
    m_root->addChild(runwayGeode.get());
}

cv::Mat SimulationBootstrapper::buildRunwayMask(int resolution) const {
    cv::Mat mask(resolution, resolution, CV_8UC1, cv::Scalar(0));

    const int runwayWidth = resolution / 10;
    const cv::Rect runwayRect(resolution / 4, (resolution - runwayWidth) / 2, resolution / 2, runwayWidth);
    cv::rectangle(mask, runwayRect, cv::Scalar(255), cv::FILLED);

    const int centerLineThickness = std::max(1, runwayWidth / 8);
    cv::line(mask,
             cv::Point(runwayRect.x, runwayRect.y + runwayRect.height / 2),
             cv::Point(runwayRect.x + runwayRect.width, runwayRect.y + runwayRect.height / 2),
             cv::Scalar(128),
             centerLineThickness,
             cv::LINE_AA);

    return mask;
}

} // namespace earth::core
