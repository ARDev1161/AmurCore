cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(AMURProtos LANGUAGES CXX)

# Option to decide whether to use FetchContent to download gRPC or use the system-installed version
set(GRPC_FETCHCONTENT true CACHE BOOL "Use FetchContent to download gRPC, or use system gRPC? yes/NO.")

include(${CMAKE_SOURCE_DIR}/cmake/grpc.cmake)

# Define the directory where all .proto files are located
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# Define the directory where the generated source and header files will be placed
set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Recursively find all .proto files within the PROTO_DIR directory and its subdirectories
file(GLOB_RECURSE PROTO_FILES "${PROTO_DIR}/*.proto")
# Create the generated directory if it doesn't exist
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

# Initialize lists to store the paths of generated files
set(PROTO_SRC_FILES)
set(PROTO_HDR_FILES)
set(GRPC_SRC_FILES)
set(GRPC_HDR_FILES)

# Iterate over each found .proto file to set up custom commands for code generation
foreach(proto_file ${PROTO_FILES})
    # Obtain the relative path of the .proto file with respect to PROTO_DIR
    file(RELATIVE_PATH REL_PROTO_FILE ${PROTO_DIR} ${proto_file})

    # Replace '/' with '_' and remove the '.proto' extension to create a unique base name
    string(REPLACE "/" "_" PROTO_NAME "${REL_PROTO_FILE}")
    string(REPLACE ".proto" "" PROTO_NAME "${PROTO_NAME}")

    # Define the output paths for the generated .pb.cc, .pb.h, .grpc.pb.cc, and .grpc.pb.h files
    set(proto_src "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.cc")
    set(proto_hdr "${GENERATED_PROTO_DIR}/${PROTO_NAME}.pb.h")
    set(grpc_src "${GENERATED_PROTO_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(grpc_hdr "${GENERATED_PROTO_DIR}/${PROTO_NAME}.grpc.pb.h")

    # Add a custom command to generate the C++ source and header files from the .proto file
    add_custom_command(
        OUTPUT "${proto_src}" "${proto_hdr}" "${grpc_src}" "${grpc_hdr}"
        COMMAND ${CMAKE_COMMAND} -E echo "=== Generating code from ${proto_file} ==="
        COMMAND ${_PROTOBUF_PROTOC}
                --grpc_out "${GENERATED_PROTO_DIR}"
                --cpp_out "${GENERATED_PROTO_DIR}"
                -I "${PROTO_DIR}"
                --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
                "${proto_file}"
        COMMENT "Protobuf compilation for ${proto_file}"
        DEPENDS "${proto_file}"
    )

    # Append the generated file paths to their respective lists
    list(APPEND PROTO_SRC_FILES "${proto_src}")
    list(APPEND PROTO_HDR_FILES "${proto_hdr}")
    list(APPEND GRPC_SRC_FILES "${grpc_src}")
    list(APPEND GRPC_HDR_FILES "${grpc_hdr}")
endforeach()

foreach(proto_src_file IN LISTS PROTO_SRC_FILES GRPC_SRC_FILES)
    set_property(SOURCE "${proto_src_file}" PROPERTY SKIP_AUTOGEN ON)
endforeach()

foreach(proto_hdr_file IN LISTS PROTO_HDR_FILES GRPC_HDR_FILES)
    set_property(SOURCE "${proto_hdr_file}" PROPERTY SKIP_AUTOGEN ON)
endforeach()

# Create a static library named BeerBotProtos from the generated files
add_library(${PROJECT_NAME} STATIC
    ${PROTO_SRC_FILES}
    ${PROTO_HDR_FILES}
    ${GRPC_SRC_FILES}
    ${GRPC_HDR_FILES}
)

# Specify include directories for the library
# GENERATED_PROTO_DIR contains the generated .pb.h and .grpc.pb.h files
# PROTO_DIR contains the original .proto files for import resolution
target_include_directories(${PROJECT_NAME} PUBLIC
    ${GENERATED_PROTO_DIR}
    ${PROTO_DIR}
)

# Link the necessary libraries to BeerBotProtos
target_link_libraries(${PROJECT_NAME}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
)
