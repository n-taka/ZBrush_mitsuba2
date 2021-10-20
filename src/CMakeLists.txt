cmake_minimum_required(VERSION 3.15)

set(PROJECT ZBrush_mitsuba2_main)
project(${PROJECT})

##################
##### target #####
##################
set(CPP_FILES
    main.cpp
    ../plugin/ZBrush_mitsuba2.cpp
    ../submodule/tinyexr/deps/miniz/miniz.c
)

file(GLOB_RECURSE RESOURCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
set(RES_FILES
    ${RESOURCES_FILES}
)

# Target for this build
add_executable(${PROJECT} ${APP_BUNDLE} ${CPP_FILES} ${RES_FILES})
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


#####################
##### Resources #####
#####################
if(WIN32)
    #####
    # Resources
    add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT}>/resources")
    add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:${PROJECT}>/resources")
elseif (APPLE)
    #####
    # Resources
    foreach(RES_FILE ${RESOURCES_FILES})
        file(RELATIVE_PATH RES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources" ${RES_FILE})
        get_filename_component(RES_DIR_PATH ${RES_PATH} DIRECTORY)
        set_property(SOURCE ${RES_FILE} PROPERTY MACOSX_PACKAGE_LOCATION "resources/${RES_DIR_PATH}")
    endforeach(RES_FILE)
endif ()
###########################
##### Resources (end) #####
###########################

