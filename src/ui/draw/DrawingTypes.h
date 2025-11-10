#pragma once

#include <vector>

namespace earth::ui::draw {

/**
 * @brief 表示贴地量测采样到的经纬高坐标，单位分别为度/度/米。
 */
struct MapGeoPoint {
    double longitudeDeg = 0.0; /**< 经度（度），范围 [-180, 180]。 */
    double latitudeDeg = 0.0;  /**< 纬度（度），范围 [-90, 90]。 */
    double altitudeMeters = 0.0; /**< 高程（米），相对平均海平面。 */
};

/**
 * @brief 贴地图形绘制工具类型。
 */
enum class DrawingTool {
    None = 0, /**< 未激活绘制工具。 */
    Point,    /**< 单点采样/标注。 */
    Polyline, /**< 折线/测距工具。 */
    Rectangle, /**< 矩形/范围框工具。 */
    Freehand /**< 自由画笔/手绘轨迹。 */
};

/**
 * @brief 表示当前正在构建的图元拓扑。
 */
enum class PrimitiveType {
    Point,
    Polyline,
    Polygon
};

/**
 * @brief RGBA 颜色定义，所有通道采用 [0,1] 浮点值。
 */
struct ColorRgba {
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;
    float a = 1.0F;
};

/**
 * @brief 记录一次绘制操作生成的顶点序列与渲染属性。
 */
struct PrimitiveDefinition {
    PrimitiveType type = PrimitiveType::Polyline; /**< 几何拓扑类型。 */
    std::vector<MapGeoPoint> vertices; /**< 顶点列表，按顺时针/采样顺序排列。 */
    ColorRgba strokeColor{}; /**< 线框颜色。 */
    double thicknessPixels = 3.0; /**< 线宽（像素）。 */
    bool filled = false; /**< 多边形是否填充。 */
    double fillOpacity = 0.35; /**< 填充不透明度，范围 [0,1]。 */
};

} // namespace earth::ui::draw
