cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(ThreadsHandler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

set(THREADS_SOURCES
    session.cpp session.h
    worker.cpp  worker.h
)

#add_library(ThreadsHandler
#            SHARED
#            ${THREADS_SOURCES}
#           )
