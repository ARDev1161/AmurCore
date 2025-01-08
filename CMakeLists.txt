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

set(GRPC_FETCHCONTENT true CACHE BOOL "Use FetchContent for download gRPC, or use system gRPC? YES/no.")
include(cmake/grpc.cmake)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/include/")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/third_party/protobuf/src/")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/_deps/grpc-src/third_party/abseil-cpp/")

# for generated pb files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql LinguistTools TextToSpeech)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql LinguistTools TextToSpeech)

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
                        SpeechProccessor

                        config++
                        ${SDL2_LIBRARIES}
                        ${OpenCV_LIBS}
                        Qt${QT_VERSION_MAJOR}::TextToSpeech
                        Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER AmurClient.online
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    if (NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR} CACHE INTERNAL "")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        set(IS_CROSSCOMP OFF CACHE INTERNAL "")
    else()
        set(IS_CROSSCOMP ON CACHE INTERNAL "")
    endif()

    set(DATA_PATH ${CMAKE_SOURCE_DIR}/data)
    set(LOGO_PATH ${DATA_PATH}/images/logo.png)
    INSTALL(CODE
        "include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/appimage.cmake)
            make_appimage(
                EXE \"${CMAKE_BINARY_DIR}/${PROJECT_NAME}\"
                NAME \"${PROJECT_NAME}\"
                ICON \"${LOGO_PATH}\"
                DIR_ICON \"${LOGO_PATH}\"
                OUTPUT_NAME \"${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}.AppImage\"
                OUTPUT_ARCH \"${CMAKE_SYSTEM_PROCESSOR}\"
                CROSSCOMP ${IS_CROSSCOMP}
                ASSETS \"${DATA_PATH}\"
            )
            "
            COMPONENT Runtime
        )
endif()
