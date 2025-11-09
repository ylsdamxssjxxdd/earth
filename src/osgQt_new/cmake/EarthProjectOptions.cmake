if(DEFINED EARTH_PROJECT_OPTIONS_INCLUDED)
    return()
endif()
set(EARTH_PROJECT_OPTIONS_INCLUDED TRUE)

option(EARTH_BUILD_EXAMPLES "Build the Qt + osgEarth example applications" ON)
option(EARTH_BUILD_TESTS "Build unit tests" OFF)

# Collect feature definitions that we want to propagate to targets.
function(earth_collect_feature_definitions out_var)
    set(defs "")
    set(${out_var} "${defs}" PARENT_SCOPE)
endfunction()
