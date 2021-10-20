cmake_minimum_required(VERSION 3.15)

set(PROJECT ZBrush_mitsuba2)
project(${PROJECT})

##################
##### target #####
##################
# Target for this build
add_library(${PROJECT} MODULE)

file(GLOB PLUGIN_CPP "*.cpp")
target_sources(
    ${PROJECT}
    PRIVATE
    ${PLUGIN_CPP}
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodule/tinyexr/deps/miniz/miniz.c
)
########################
##### target (end) #####
########################


#####################
##### Libraries #####
#####################
# Add external libraries
target_link_libraries(${PROJECT} PRIVATE ${LINK_LIBS})
###########################
##### Libraries (end) #####
###########################


###################
##### Include #####
###################
# Add a include files
target_include_directories(${PROJECT} PUBLIC ${INCLUDE_DIR})
#########################
##### Include (end) #####
#########################


###########################
##### compile options #####
###########################
if (WIN32)
    # because we want to use std::filesystem, we require C++17 for windows
    target_compile_features(${PROJECT} PUBLIC cxx_std_17)
elseif (APPLE)
    # because we support macOS Mojave, we only require C++14 for macOS
    # we use boost::filesystem for macOS
    target_compile_features(${PROJECT} PUBLIC cxx_std_14)
endif ()
target_compile_definitions(${PROJECT} PUBLIC _USE_MATH_DEFINES)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4819")
    add_definitions(-D_WIN32_WINNT=0x0A00)
endif ()
#################################
##### compile options (end) #####
#################################
