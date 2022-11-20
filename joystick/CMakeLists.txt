cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(JoystickHandler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(JOYSTICK_SOURCES
    v_joystick_adapter.cpp     v_joystick_adapter.h
    getstatebyjoy.cpp          getstatebyjoy.h
    joystickstateworker.cpp    joystickstateworker.h
    joystick.cpp               joystick.h

    joystickdialog.cpp joystickdialog.h
    joystickdialog.ui
)

add_library(JoystickHandler
            SHARED
            ${JOYSTICK_SOURCES}
           )

target_link_libraries(JoystickHandler PRIVATE
                                      Qt${QT_VERSION_MAJOR}::Widgets
                                  )
