cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(CameraSetup LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(CAMERA_SOURCES
    calibrator.cpp           calibrator.h
    calibratorworker.cpp     calibratorworker.h
    camsettingsholder.cpp    camsettingsholder.h

    camcalibrate.cpp    camcalibrate.h
    camcalibrate.ui
)

add_library(${PROJECT_NAME}
            SHARED
            ${CAMERA_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PRIVATE
                    Qt${QT_VERSION_MAJOR}::Core
                    Qt${QT_VERSION_MAJOR}::Widgets
                    ${OpenCV_LIBS}
                )
