#pragma once

#include <osg/Group>
#include <osg/Node>
#include <osg/observer_ptr>
#include <osg/ref_ptr>
#include <osgEarth/Map>
#include <osgEarth/Sky>
#include <opencv2/core.hpp>
#include <memory>

namespace osgEarth {
class MapNode;
class SpatialReference;
} // namespace osgEarth

namespace earth::core {

/**
 * @brief 负责机场仿真的基础资源加载，构建 osgEarth 地图、SkyNode 以及 Qt 渲染所需的场景根节点。
 *
 * 该引导器在应用启动时创建默认机场场景，并在加载外部 .earth 文件时保持天空、光照、星空等环境效果与参考工程一致。
 */
class SimulationBootstrapper {
public:
    SimulationBootstrapper();

    /**
     * @brief 执行一次性初始化，构建默认场景并缓存 OpenCV 需要的跑道掩码。
     */
    void initialize();

    /**
     * @brief 返回可直接设置给 osgViewer::View 的场景根节点。
     */
    osg::Group* sceneRoot() const;

    /**
     * @brief 获取跑道区域掩码图，供视觉算法或调试面板复用。
     */
    cv::Mat runwayMask() const;

    /**
     * @brief 暴露当前激活的 SkyNode，方便 SceneWidget 将其 attach 到 viewer 以驱动光照与星空。
     */
    osgEarth::SkyNode* skyNode() const;

    /**
     * @brief 将 .earth 文件加载得到的场景并入当前框架，自动接管天空与环境设置。
     * @param externalScene osgDB::readNodeFile 返回的根节点，必须包含 MapNode。
     * @return 成功接入返回 true，若缺失 MapNode 或 scene graph 非法则返回 false。
     */
    bool applyExternalScene(osg::Node* externalScene);

private:
    /**
     * @brief 构建默认机场场景，包含基础 MapNode 与示例跑道几何。
     */
    void buildSceneGraph();

    /**
     * @brief 生成指定分辨率的跑道掩码，用于视觉算法或材质调试。
     */
    cv::Mat buildRunwayMask(int resolution) const;

    /**
     * @brief 根据地图空间参考配置 SkyNode 驱动、品质、时间等参数。
     */
    std::unique_ptr<osgEarth::SkyOptions> buildSkyOptions(const osgEarth::SpatialReference* srs) const;

    /**
     * @brief 复用或安装新的 SkyNode，使其与最新 MapNode 保持一致的天空/星空表现。
     */
    void configureSky(osgEarth::MapNode* mapNode);

    /**
     * @brief 刷新根节点层级，确保 scene container 始终在 SkyNode 之下或单独暴露给 Qt。
     */
    void rebuildSceneGraph();

    osg::ref_ptr<osg::Group> m_root;
    osg::ref_ptr<osgEarth::Map> m_map;
    osg::ref_ptr<osg::Group> m_sceneContainer;
    osg::ref_ptr<osgEarth::SkyNode> m_sky;
    osg::observer_ptr<osgEarth::SkyNode> m_externalSky;
    osg::observer_ptr<osgEarth::MapNode> m_activeMapNode;
    cv::Mat m_cachedRunwayMask;
};

} // namespace earth::core
