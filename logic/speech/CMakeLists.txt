cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(SpeechProcessor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5TextToSpeech)
find_package(Qt5Widgets)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(SPEECH_SOURCES
    prim_type.h
    sphinxrecognizer.cpp   sphinxrecognizer.h
    speechdialog.cpp speechdialog.h
    speechdialog.ui
)

