cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(AMURProtos LANGUAGES CXX)

# set(GRPC_FETCHCONTENT true CACHE BOOL "Use FetchContent for download gRPC, or use system gRPC? yes/NO.")

# include(grpc_common.cmake)

set(PROTO_NAME amur)
# Proto file
get_filename_component(hw_proto "${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_NAME}.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(PROTOS_LIB
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h

    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
    )

add_library(${PROJECT_NAME}
            STATIC 
            ${PROTOS_LIB}
           )

target_link_libraries(${PROJECT_NAME}
                      ${ATOMIC_LIBRARY}
                      ${_REFLECTION}
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF}
                      )

