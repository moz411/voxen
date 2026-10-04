add_library(voxen_engine STATIC
    core/src/Engine.cpp
)

target_include_directories(voxen_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/xr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/render/include
)

target_compile_features(voxen_engine PUBLIC cxx_std_20)
