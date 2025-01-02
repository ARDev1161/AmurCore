cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(SpeechProccessor LANGUAGES CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(SPEECH_SOURCES
    prim_type.h
    sphinxrecognizer.cpp   sphinxrecognizer.h
    speechdialog.cpp speechdialog.h
    speechdialog.ui
)

add_library(${PROJECT_NAME}
            STATIC
            ${SPEECH_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::TextToSpeech)

target_link_libraries(${PROJECT_NAME} PRIVATE
                    Qt${QT_VERSION_MAJOR}::Widgets
                )
