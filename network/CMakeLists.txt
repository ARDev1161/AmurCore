cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(NetworkController LANGUAGES CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

add_subdirectory(protobuf)

set(CLIENT_LIB 
    client.cpp 
    client.h
   )
   
set(SERVER_LIB
    server.cpp
    server.h
   )

set(CONNECT_DIALOG
    connectdialog.cpp   connectdialog.h
    connectdialog.ui
   )

set(NETWORK_LIB 
    networkcontroller.cpp 
    networkcontroller.h 
    ${CLIENT_LIB} 
    ${SERVER_LIB}
    ${CONNECT_DIALOG}
    ${PROTOS_LIB}
   )

include_directories("${CMAKE_BINARY_DIR}")

add_library(${PROJECT_NAME}
            STATIC
            ${NETWORK_LIB}
            )

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::Network)

target_link_libraries(${PROJECT_NAME} PRIVATE
                    AMURProtos
                    Qt${QT_VERSION_MAJOR}::Widgets
                )

