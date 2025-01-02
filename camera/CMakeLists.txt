cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(CameraSetup LANGUAGES CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

#find_package( OpenCV REQUIRED )
#include_directories( ${OpenCV_INCLUDE_DIRS} )

set(CAMERA_SOURCES
    calibrator.cpp           calibrator.h
    calibratorworker.cpp     calibratorworker.h
    camsettingsholder.cpp    camsettingsholder.h

    camcalibrate.cpp    camcalibrate.h
    camcalibrate.ui
)

add_library(${PROJECT_NAME}
            STATIC
            ${CAMERA_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PRIVATE
                    Qt${QT_VERSION_MAJOR}::Widgets
                    ${OpenCV_LIBS}
                )
