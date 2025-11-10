#include "core/SimulationBootstrapper.h"

#include "core/EnvironmentBootstrapper.h"
#include <algorithm>
#include <osg/Geode>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/StateAttribute>
#include <osg/Vec3>
#include <osgEarth/DateTime>
#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osgEarth/NodeUtils>
#include <osgEarth/SpatialReference>
#include <osgEarth/URI>
#include <osgEarthDrivers/sky_simple/SimpleSkyOptions>
#include <opencv2/imgproc.hpp>

namespace earth::core {
namespace {
} // namespace

SimulationBootstrapper::SimulationBootstrapper()
    : m_root(new osg::Group())
    , m_map(new osgEarth::Map())
    , m_sceneContainer(new osg::Group()) {
    if (m_sceneContainer.valid()) {
        m_sceneContainer->setName("SceneContainer");
    }
}

void SimulationBootstrapper::initialize() {
    EnvironmentBootstrapper::instance().initialize();
    buildSceneGraph();
    m_cachedRunwayMask = buildRunwayMask(128);
}

osg::Group* SimulationBootstrapper::sceneRoot() const {
    return m_root.get();
}

cv::Mat SimulationBootstrapper::runwayMask() const {
    return m_cachedRunwayMask;
}

osgEarth::SkyNode* SimulationBootstrapper::skyNode() const {
    if (m_sky.valid()) {
        return m_sky.get();
    }
    if (m_externalSky.valid()) {
        return m_externalSky.get();
    }
    return nullptr;
}

bool SimulationBootstrapper::applyExternalScene(osg::Node* externalScene) {
    if (!externalScene || !m_sceneContainer.valid()) {
        return false;
    }

    m_sceneContainer->removeChildren(0, m_sceneContainer->getNumChildren());
    m_sceneContainer->addChild(externalScene);

    osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(externalScene);
    configureSky(mapNode);
    rebuildSceneGraph();
    return mapNode != nullptr;
}

void SimulationBootstrapper::buildSceneGraph() {
    if (!m_sceneContainer.valid()) {
        m_sceneContainer = new osg::Group();
    }
    m_sceneContainer->setName("SceneContainer");
    m_sceneContainer->removeChildren(0, m_sceneContainer->getNumChildren());

    osg::ref_ptr<osgEarth::MapNode> mapNode = new osgEarth::MapNode(m_map.get());
    mapNode->setName("AirportMapNode");
    m_sceneContainer->addChild(mapNode.get());
    m_activeMapNode = mapNode.get();

    osg::ref_ptr<osg::Geode> runwayGeode = new osg::Geode();
    osg::ref_ptr<osg::ShapeDrawable> runwayGeometry =
        new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 0.0f), 1000.0f, 60.0f, 2.0f));
    runwayGeometry->setName("ProceduralRunway");
    runwayGeode->addDrawable(runwayGeometry.get());
    m_sceneContainer->addChild(runwayGeode.get());

    configureSky(mapNode.get());
    rebuildSceneGraph();
}

osgEarth::MapNode* SimulationBootstrapper::activeMapNode() const {
    if (m_activeMapNode.valid()) {
        return m_activeMapNode.get();
    }
    return nullptr;
}

cv::Mat SimulationBootstrapper::buildRunwayMask(int resolution) const {
    cv::Mat mask(resolution, resolution, CV_8UC1, cv::Scalar(0));

    const int runwayWidth = resolution / 10;
    const cv::Rect runwayRect(resolution / 4, (resolution - runwayWidth) / 2, resolution / 2, runwayWidth);

    cv::Mat roiRunway = mask(runwayRect);
    roiRunway.setTo(cv::Scalar(255));

    const int centerLineThickness = std::max(1, runwayWidth / 8);
    const int centerY = runwayRect.y + runwayRect.height / 2 - centerLineThickness / 2;
    cv::Rect centerRect(runwayRect.x, std::max(0, centerY), runwayRect.width, centerLineThickness);
    centerRect &= cv::Rect(0, 0, mask.cols, mask.rows);
    if (centerRect.width > 0 && centerRect.height > 0) {
        cv::Mat roiCenter = mask(centerRect);
        roiCenter.setTo(cv::Scalar(128));
    }

    return mask;
}

std::unique_ptr<osgEarth::SkyOptions> SimulationBootstrapper::buildSkyOptions(const osgEarth::SpatialReference* srs) const {
    const bool useSimpleSky = (srs == nullptr) || srs->isGeographic();
    const auto configureCommon = [](osgEarth::SkyOptions& opts) {
        opts.quality() = osgEarth::SkyOptions::QUALITY_LOW;
        opts.ambient() = 0.08f;
        opts.hours() = 22.0f;
    };

    if (useSimpleSky) {
        auto options = std::make_unique<osgEarth::SimpleSky::SimpleSkyOptions>();
        configureCommon(*options);
        options->setDriver("simple");

        const std::string moonPath = EnvironmentBootstrapper::instance().moonTextureFile();
        if (!moonPath.empty()) {
            options->moonImageURI() = osgEarth::URI(moonPath);
        }
        return options;
    }

    auto options = std::make_unique<osgEarth::SkyOptions>();
    configureCommon(*options);
    options->setDriver("gl");
    return options;
}

void SimulationBootstrapper::configureSky(osgEarth::MapNode* mapNode) {
    m_activeMapNode = mapNode;

    if (!mapNode) {
        m_sky = nullptr;
        m_externalSky = nullptr;
        return;
    }

    osgEarth::SkyNode* embeddedSky = nullptr;
    if (m_sceneContainer.valid()) {
        embeddedSky = osgEarth::findTopMostNodeOfType<osgEarth::SkyNode>(m_sceneContainer.get());
    }
    if (!embeddedSky) {
        embeddedSky = osgEarth::findTopMostNodeOfType<osgEarth::SkyNode>(mapNode);
    }

    if (embeddedSky != nullptr) {
        m_externalSky = embeddedSky;
        m_sky = nullptr;
        embeddedSky->setSunVisible(true);
        embeddedSky->setMoonVisible(true);
        embeddedSky->setStarsVisible(true);
        embeddedSky->setAtmosphereVisible(true);
        return;
    }

    m_externalSky = nullptr;
    std::unique_ptr<osgEarth::SkyOptions> options = buildSkyOptions(mapNode->getMapSRS());
    if (!options) {
        return;
    }
    m_sky = osgEarth::SkyNode::create(*options);
    if (!m_sky.valid()) {
        return;
    }

    m_sky->setName("AtmosphereSkyNode");
    m_sky->setDateTime(osgEarth::DateTime(2021, 4, 21, 22.0));
    m_sky->setSunVisible(true);
    m_sky->setMoonVisible(true);
    m_sky->setStarsVisible(true);
    m_sky->setAtmosphereVisible(true);
    m_sky->setSimulationTimeTracksDateTime(true);
    m_sky->setLighting(osg::StateAttribute::ON);

    if (const auto* mapSRS = mapNode->getMapSRS(); mapSRS && mapSRS->isProjected()) {
        osgEarth::GeoPoint refPoint(mapSRS, 0.0, 0.0, 0.0, osgEarth::ALTMODE_ABSOLUTE);
        m_sky->setReferencePoint(refPoint);
    }
}

void SimulationBootstrapper::rebuildSceneGraph() {
    if (!m_root.valid()) {
        return;
    }

    m_root->removeChildren(0, m_root->getNumChildren());

    if (m_sky.valid()) {
        m_sky->removeChildren(0, m_sky->getNumChildren());
        if (m_sceneContainer.valid()) {
            m_sky->addChild(m_sceneContainer.get());
        }
        m_root->addChild(m_sky.get());
    } else if (m_sceneContainer.valid()) {
        m_root->addChild(m_sceneContainer.get());
    }
}

} // namespace earth::core
