cmake_minimum_required(VERSION 3.5)

project(AmurCore VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

#option(AMURCLIENT_TESTING "Включить модульное тестирование" ON)
#option(AMURCLIENT_COVERAGE "Включить измерение покрытия кода тестами" OFF)

#enable_testing()
#find_package(GTest REQUIRED)

if(NOT IS_SUBPROJECT)
    find_package(Doxygen)
    message(STATUS "Doxygen ${DOXYGEN_FOUND} - version ${DOXYGEN_VERSION} ")
    message(STATUS "Doxygen dot ${DOXYGEN_DOT_FOUND}")

#    add_subdirectory(doc)

    if (Doxygen_FOUND)
        if (NOT MYLIB_DOXYGEN_LANGUAGE)
            set(MYLIB_DOXYGEN_LANGUAGE Russian)
        endif()

        message(STATUS "Doxygen documentation will be generated in ${MYLIB_DOXYGEN_LANGUAGE}")

        add_custom_target(doc COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    endif ()
endif()

add_compile_options(
#    -Werror

    -Wall
    -Wextra
    -Wpedantic

    -Wcast-align
    -Wcast-qual
#    -Wconversion
    -Wsign-conversion
#    -Wsign-promo
    -Wdisabled-optimization
    -Wctor-dtor-privacy
    -Wformat=2
    -Winit-self
    -Wlogical-op
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Wnoexcept
#    -Wshadow
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wstrict-null-sentinel
    -Wstrict-overflow=5
    -Wswitch-default
    -Wundef
    -Wno-unused
)

if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

find_library(libСonfig_LIB config++)

# Copy config from source to build
configure_file(
        AmurCore.cfg.in
        AmurCore.cfg
        )

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(TS_FILES AmurCore_ru_RU.ts)

set(IMAGES
    data/images/no_picture.jpeg
)

add_subdirectory(camera)
add_subdirectory(network)
add_subdirectory(joystick)
add_subdirectory(threads)
add_subdirectory(logic)

set(PROJECT_SOURCES
        main.cpp
        pch.h
        amurcore.cpp    amurcore.h
        AmurCore.ui

        ${TS_FILES}
        ${IMAGES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AmurCore
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET AmurCore APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(AmurCore SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(AmurCore
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(AmurCore PRIVATE
                        LogicCore
                        CameraSetup
                        ThreadsHandler
                        JoystickHandler
                        NetworkController

                        config++
                        ${SDL2_LIBRARIES}
                        ${OpenCV_LIBS}
                        Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(AmurCore PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER AmurClient.online
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AmurCore)
endif()
