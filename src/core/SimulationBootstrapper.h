#pragma once

#include <osg/Group>
#include <osg/ref_ptr>
#include <osgEarth/Map>
#include <opencv2/core.hpp>

namespace earth::core {

/**
 * @brief 机场仿真场景初始化器，负责搭建基础osgEarth场景并准备视觉数据。
 */
class SimulationBootstrapper {
public:
    SimulationBootstrapper();

    /**
     * @brief 执行一次性初始化，包括地形、跑道以及相关诊断数据。
     */
    void initialize();

    /**
     * @brief 提供场景图根节点，供Qt视窗或其他模块挂接。
     */
    osg::Group* sceneRoot() const;

    /**
     * @brief 获取缓存的跑道掩码图，后续模块可直接复用。
     */
    cv::Mat runwayMask() const;

private:
    /**
     * @brief 创建基础地图节点与临时可视对象。
     */
    void buildSceneGraph();

    /**
     * @brief 生成指定分辨率的跑道掩码，便于后续雷达与可视化模块共享。
     */
    cv::Mat buildRunwayMask(int resolution) const;

    osg::ref_ptr<osg::Group> m_root;
    osg::ref_ptr<osgEarth::Map> m_map;
    cv::Mat m_cachedRunwayMask;
};

} // namespace earth::core
