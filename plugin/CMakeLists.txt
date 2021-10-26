cmake_minimum_required(VERSION 3.15)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

set(PROJECT mitsuba2_shooter)
project(${PROJECT})

##################
##### target #####
##################
# Target for this build
add_library(${PROJECT} SHARED)

target_sources(
    ${PROJECT}
    PRIVATE
    mitsuba2_shooter.cpp
    appendMitsubaBaseShape.cpp
    appendMitsubaEmitter.cpp
    appendMitsubaFilm.cpp
    appendMitsubaIntegrator.cpp
    appendMitsubaSampler.cpp
    appendMitsubaScene.cpp
    appendMitsubaSensor.cpp
    appendMitsubaTargetShape.cpp
    exportImage.cpp
    getMinimumBoundingSphere.cpp
    readGoZAndTriangulate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/tinyexr/deps/miniz/miniz.c
)
########################
##### target (end) #####
########################


#####################
##### Libraries #####
#####################
###
# json
find_package(nlohmann_json CONFIG REQUIRED)

###
# eigen3
find_package(Eigen3 CONFIG REQUIRED)

###
# libigl
find_package(libigl CONFIG REQUIRED)

# Add external libraries
target_link_libraries(
    ${PROJECT}
    PRIVATE
    nlohmann_json
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
    igl::core
    igl::common
)
###########################
##### Libraries (end) #####
###########################


###################
##### Include #####
###################
# Add a include files
target_include_directories(
    ${PROJECT}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/stb_image
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/tinyexr
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/tinyexr/deps/miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/rapidxml-1.13
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/FromZ/src
)
#########################
##### Include (end) #####
#########################


###########################
##### compile options #####
###########################
if (WIN32)
    # because we want to use std::filesystem, we require C++17 for windows
    target_compile_features(${PROJECT} PRIVATE cxx_std_17)
elseif (APPLE)
    # because we support macOS Mojave, we only require C++14 for macOS
    # we use boost::filesystem for macOS
    target_compile_features(${PROJECT} PRIVATE cxx_std_14)
endif ()
target_compile_definitions(${PROJECT} PRIVATE _USE_MATH_DEFINES)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4819")
    add_definitions(-D_WIN32_WINNT=0x0A00)
endif ()
#################################
##### compile options (end) #####
#################################
