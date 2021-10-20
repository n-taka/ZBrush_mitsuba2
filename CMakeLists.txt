cmake_minimum_required(VERSION 3.15)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if(APPLE)
    # at least we support macOS that is still maintained
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "" FORCE)
endif ()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/submodule/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(PROJECT ZBrush_mitsuba2_TopLevel)
project(${PROJECT})

# version
set(Version "1.0")
# basically, we use Release
if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

#####################
##### Libraries #####
#####################
###
# json
find_package(nlohmann_json CONFIG REQUIRED)

###
# eigen3
find_package(Eigen3 3.3 REQUIRED)

set(LINK_LIBS
    nlohmann_json
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
)
# add OS specific libraries
if(WIN32)
    # we need to explicitly add bcrypt.lib due to some bug.
    set(LINK_LIBS
        ${LINK_LIBS}
    )
elseif (APPLE)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    set(LINK_LIBS
        ${LINK_LIBS}
        ${CORE_FOUNDATION_LIBRARY}
    )
endif ()
###########################
##### Libraries (end) #####
###########################


###################
##### Include #####
###################
# Include directories for this project
set(INCLUDE_DIR
    ${EIGEN3_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/stb_image
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/tinyexr
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/tinyexr/deps/miniz
    ${CMAKE_CURRENT_SOURCE_DIR}/submodule/rapidxml-1.13
)
#########################
##### Include (end) #####
#########################


###############################
##### app bundle settings #####
###############################
if(APPLE)
    set(APP_BUNDLE MACOSX_BUNDLE)
endif()

if(APPLE)
    set(MACOSX_BUNDLE_EXECUTABLE ${PROJECT})
    set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "info.n-taka")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} Version ${Version}")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${Version})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${Version})
    # set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2020--2021.")
endif()
###############################
##### app bundle settings #####
###############################

add_subdirectory(src)
add_subdirectory(plugin)
