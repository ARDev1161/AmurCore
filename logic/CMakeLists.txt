cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(LogicCore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

#add_subdirectory(speech)

set(LOGIC_SOURCES
    movements.cpp movements.h
    system.cpp    system.h
    logic.cpp     logic.h

#    ${SPEECH_SOURCES}
)

add_library(LogicCore
            SHARED
            ${LOGIC_SOURCES}
           )

#target_link_libraries(LogicCore SpeechProcessor)
