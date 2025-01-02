cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(JoystickHandler LANGUAGES CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(JOYSTICK_SOURCES
    v_joystick_adapter.cpp     v_joystick_adapter.h
    getstatebyjoy.cpp          getstatebyjoy.h
    joystickstateworker.cpp    joystickstateworker.h
    joystick.cpp               joystick.h

    joystickdialog.cpp joystickdialog.h
    joystickdialog.ui
)

add_library(${PROJECT_NAME}
            STATIC
            ${JOYSTICK_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PRIVATE
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Widgets
           )
