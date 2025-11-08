option(EARTH_ENABLE_SCENE "启用场景模块以负责机场地形和模型管理" ON)
option(EARTH_ENABLE_RADAR "启用雷达模拟模块" ON)
option(EARTH_ENABLE_WEATHER "启用天气场仿真模块" ON)
option(EARTH_ENABLE_WIND "启用风场模块" ON)
option(EARTH_ENABLE_AIRTRAFFIC "启用空管交通模块" ON)
option(EARTH_ENABLE_DATABRIDGE "启用数据桥接模块" ON)
option(EARTH_ENABLE_PERF "启用性能诊断模块" ON)
option(EARTH_ENABLE_TOOLING "启用开发者工具模块" ON)
option(EARTH_ENABLE_EDITING "启用静态绘制与编辑模块" ON)
option(EARTH_BUILD_TESTS "构建单元测试与集成测试" OFF)

function(earth_collect_feature_definitions out_var)
    set(_defs
        $<$<BOOL:${EARTH_ENABLE_SCENE}>:EARTH_ENABLE_SCENE>
        $<$<BOOL:${EARTH_ENABLE_RADAR}>:EARTH_ENABLE_RADAR>
        $<$<BOOL:${EARTH_ENABLE_WEATHER}>:EARTH_ENABLE_WEATHER>
        $<$<BOOL:${EARTH_ENABLE_WIND}>:EARTH_ENABLE_WIND>
        $<$<BOOL:${EARTH_ENABLE_AIRTRAFFIC}>:EARTH_ENABLE_AIRTRAFFIC>
        $<$<BOOL:${EARTH_ENABLE_DATABRIDGE}>:EARTH_ENABLE_DATABRIDGE>
        $<$<BOOL:${EARTH_ENABLE_PERF}>:EARTH_ENABLE_PERF>
        $<$<BOOL:${EARTH_ENABLE_TOOLING}>:EARTH_ENABLE_TOOLING>
        $<$<BOOL:${EARTH_ENABLE_EDITING}>:EARTH_ENABLE_EDITING>
    )
    set(${out_var} "${_defs}" PARENT_SCOPE)
endfunction()
