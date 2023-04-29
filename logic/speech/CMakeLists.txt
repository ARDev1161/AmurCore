cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(SpeechProccessor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(SPEECH_SOURCES
    prim_type.h
    sphinxrecognizer.cpp   sphinxrecognizer.h
    speechdialog.cpp speechdialog.h
    speechdialog.ui
)

add_library(${PROJECT_NAME}
            SHARED
            ${SPEECH_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::TextToSpeech)

target_link_libraries(${PROJECT_NAME} PRIVATE
                    Qt${QT_VERSION_MAJOR}::Widgets
                )
