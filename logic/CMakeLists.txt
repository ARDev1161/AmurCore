cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(LogicCore LANGUAGES CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(LOGIC_SOURCES
    movements.cpp movements.h
    system.cpp    system.h
    logic.cpp     logic.h

    mapwidget.h mapwidget.cpp
    navdialog.h   navdialog.cpp
    navdialog.ui

    robotinfodialog.h   robotinfodialog.cpp
    robotinfodialog.ui
)

add_library(${PROJECT_NAME}
            STATIC
            ${LOGIC_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PRIVATE
                    JoystickHandler
                    AMURProtos
                    ${OpenCV_LIBS}
                    Qt${QT_VERSION_MAJOR}::Widgets
                )
