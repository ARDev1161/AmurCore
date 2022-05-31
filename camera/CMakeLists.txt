cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(CameraSetup LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(CAMERA_SOURCES
    calibrator.cpp           calibrator.h
    calibratorworker.cpp     calibratorworker.h
    camsettingsholder.cpp    camsettingsholder.h

    camcalibrate.cpp    camcalibrate.h
    camcalibrate.ui
)

#add_library(CameraSetup
#            SHARED
#            ${CAMERA_SOURCES}
#           )
