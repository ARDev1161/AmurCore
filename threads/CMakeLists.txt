cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(ThreadsHandler LANGUAGES CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(THREADS_SOURCES
    session.cpp session.h
    worker.cpp  worker.h
)

add_library(ThreadsHandler
            STATIC
            ${THREADS_SOURCES}
           )

target_link_libraries(${PROJECT_NAME} PRIVATE
            Qt${QT_VERSION_MAJOR}::Core
           )
