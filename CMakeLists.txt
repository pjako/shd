cmake_minimum_required(VERSION 3.21)
project(RX_SHD)

# Fetch base lib
if(PROJECT_IS_TOP_LEVEL)
    include(FetchContent)

    FetchContent_Declare(
        base
        GIT_REPOSITORY https://github.com/pjako/base.git
        GIT_TAG        main
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/base"
    )
    FetchContent_MakeAvailable(base)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/base)
endif()

add_library(spirv_cross STATIC
    shd/spirv_cross/spirv_cfg.cpp
    shd/spirv_cross/spirv_cpp.cpp
    shd/spirv_cross/spirv_cross_c.cpp
    shd/spirv_cross/spirv_cross_parsed_ir.cpp
    shd/spirv_cross/spirv_cross_util.cpp
    shd/spirv_cross/spirv_cross.cpp
    shd/spirv_cross/spirv_glsl.cpp
    shd/spirv_cross/spirv_hlsl.cpp
    shd/spirv_cross/spirv_msl.cpp
    shd/spirv_cross/spirv_parser.cpp
    shd/spirv_cross/spirv_reflect.cpp
)
set_property(TARGET spirv_cross PROPERTY CXX_STANDARD 20)

add_executable(shdc shd/dxc_wrapper.cc shd/shd_main.cc)
set_property(TARGET shdc PROPERTY CXX_STANDARD 20)
target_link_libraries(shdc base os parser log spirv_cross)

