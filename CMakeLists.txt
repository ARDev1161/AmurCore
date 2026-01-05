cmake_minimum_required(VERSION 3.16)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GetVersionFromGit.cmake")
project(AmurCore VERSION ${PROJECT_VERSION_SEMVER} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# add_compile_options(
# #    -Werror

#     -Wall
#     -Wextra
#     -Wpedantic

#     -Wcast-align
#     -Wcast-qual
# #    -Wconversion
#     -Wsign-conversion
# #    -Wsign-promo
#     -Wdisabled-optimization
#     -Wctor-dtor-privacy
#     -Wformat=2
#     -Winit-self
#     -Wlogical-op
#     -Wmissing-declarations
#     -Wmissing-include-dirs
#     -Wnoexcept
# #    -Wshadow
#     -Wenum-compare
#     -Wfloat-equal
#     -Wnon-virtual-dtor
#     -Wold-style-cast
#     -Woverloaded-virtual
#     -Wredundant-decls
#     -Wstrict-null-sentinel
#     -Wstrict-overflow=5
#     -Wswitch-default
#     -Wundef
#     -Wno-unused
# )

find_library(libСonfig_LIB config++)

# Copy config from source to build
configure_file(
        AmurCore.cfg.in
        AmurCore.cfg
        )

#set(GRPC_FETCHCONTENT true CACHE BOOL "Use FetchContent for download gRPC, or use system gRPC? YES/no.")
#include(cmake/grpc.cmake)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/include/")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/third_party/protobuf/src/")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/third_party/abseil-cpp/")

# for generated pb files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(QT_VERSION_MAJOR 5)
find_package(Qt5 REQUIRED COMPONENTS Widgets Network Sql LinguistTools)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(OpenCV REQUIRED)
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
add_subdirectory(config)

set(CONFIG_PROCESSOR
    config/configprocessor.h
    config/configprocessor.cpp
   )

set(PROJECT_SOURCES
        main.cpp
        pch.h
        amurcore.cpp    amurcore.h
        AmurCore.ui

        ${CONFIG_PROCESSOR}

        ${TS_FILES}
        ${IMAGES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

file(COPY "${CMAKE_SOURCE_DIR}/data"
     DESTINATION "${CMAKE_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE
                        LogicCore
                        CameraSetup
                        ThreadsHandler
                        JoystickHandler
                        NetworkController

                        config++
                        ${SDL2_LIBRARIES}
                        Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER AmurClient.online
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION_SEMVER}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_SEMVER_MAJOR}.${PROJECT_VERSION_SEMVER_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# Optionally add an option to enable/disable AppImage build
option(BUILD_APPIMAGE "Generate .AppImage using linuxdeploy" ON)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    include("${CMAKE_SOURCE_DIR}/cmake/linuxdeploy.cmake")

    set(APPIMAGE_QMAKE "")
    find_program(QMAKE_PROGRAM NAMES qmake-qt5 qmake)
    if(QMAKE_PROGRAM)
        set(APPIMAGE_QMAKE "${QMAKE_PROGRAM}")
    endif()
    if(APPIMAGE_QMAKE STREQUAL "" AND TARGET Qt5::qmake)
        get_target_property(QMAKE_LOCATION Qt5::qmake IMPORTED_LOCATION)
        if(QMAKE_LOCATION AND NOT QMAKE_LOCATION MATCHES "-NOTFOUND$")
            set(APPIMAGE_QMAKE "${QMAKE_LOCATION}")
        endif()
    endif()
    if(APPIMAGE_QMAKE STREQUAL "")
        if(DEFINED Qt5Core_QMAKE_EXECUTABLE AND NOT Qt5Core_QMAKE_EXECUTABLE MATCHES "-NOTFOUND$")
            if(Qt5Core_QMAKE_EXECUTABLE MATCHES "^Qt5::")
                if(TARGET ${Qt5Core_QMAKE_EXECUTABLE})
                    get_target_property(QMAKE_LOCATION ${Qt5Core_QMAKE_EXECUTABLE} IMPORTED_LOCATION)
                    if(QMAKE_LOCATION AND NOT QMAKE_LOCATION MATCHES "-NOTFOUND$")
                        set(APPIMAGE_QMAKE "${QMAKE_LOCATION}")
                    endif()
                endif()
            else()
                set(APPIMAGE_QMAKE "${Qt5Core_QMAKE_EXECUTABLE}")
            endif()
        elseif(DEFINED QT_QMAKE_EXECUTABLE AND NOT QT_QMAKE_EXECUTABLE MATCHES "-NOTFOUND$")
            set(APPIMAGE_QMAKE "${QT_QMAKE_EXECUTABLE}")
        endif()
    endif()

    set(APPIMAGE_QMAKE_LINE "")
    if(NOT APPIMAGE_QMAKE STREQUAL "")
        set(APPIMAGE_QMAKE_LINE "set(ENV{QMAKE} \"${APPIMAGE_QMAKE}\")\n")
    endif()

    file(WRITE "${CMAKE_BINARY_DIR}/appimage_invoke.cmake" "
include(\"${CMAKE_SOURCE_DIR}/cmake/linuxdeploy.cmake\")
set(ENV{NO_STRIP} \"1\")
set(ENV{LINUXDEPLOY_EXCLUDED_LIBRARIES} \"libsybdb.so.5\")
${APPIMAGE_QMAKE_LINE}

build_appimage(
    APP_NAME         \"${PROJECT_NAME}\"
    APP_VERSION      \"${PROJECT_VERSION_SEMVER}\"
    EXECUTABLE       \"${PROJECT_NAME}\"
    ICON             \"${CMAKE_SOURCE_DIR}/data/images/logo.png\"
    APPRUN           \"${CMAKE_SOURCE_DIR}/cmake/apprun.sh\"
    APP_ARCH         \"${CMAKE_SYSTEM_PROCESSOR}\"
)
")
    add_custom_target(appimage ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Building AppImage..."
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/appimage_invoke.cmake"
        DEPENDS ${PROJECT_NAME}
    )
endif()
