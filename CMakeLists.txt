cmake_minimum_required(VERSION 3.14)
project(tar_project)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED PATHS /opt/homebrew/lib/cmake/protobuf NO_DEFAULT_PATH)
find_package(gRPC REQUIRED PATHS /opt/homebrew/lib/cmake/grpc NO_DEFAULT_PATH)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(proto)

# Ensure the gRPC plugin is available
set(_PROTOBUF_PROTOC_EXECUTABLE /opt/homebrew/bin/protoc)
set(_GRPC_CPP_PLUGIN_EXECUTABLE /opt/homebrew/bin/grpc_cpp_plugin)

# Process .proto files
file(GLOB PROTO_FILES proto/*.proto)

foreach(proto_file ${PROTO_FILES})
  get_filename_component(proto_name ${proto_file} NAME_WE)

  # Generate Protobuf files
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h
    COMMAND ${_PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${proto_file}
    DEPENDS ${proto_file}
  )

  # Generate gRPC files
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
    COMMAND ${_PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR} --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${proto_file}
    DEPENDS ${proto_file}
  )

  list(APPEND PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc)
  list(APPEND PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h)
  list(APPEND GRPC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc)
  list(APPEND GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h)
endforeach()

# Create the library for the generated Protobuf and gRPC files
add_library(tar_proto ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})
target_include_directories(tar_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${Protobuf_INCLUDE_DIRS} /opt/homebrew/include)

add_executable(server src/server_main.cpp src/TARServiceImpl.cpp src/TARAlgorithm.cpp)
add_executable(client src/client_main.cpp)

target_include_directories(server PUBLIC ${Protobuf_INCLUDE_DIRS} /opt/homebrew/include)
target_include_directories(client PUBLIC ${Protobuf_INCLUDE_DIRS} /opt/homebrew/include)

# Link gRPC reflection library to the server target
target_link_libraries(server tar_proto gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf)
target_link_libraries(client tar_proto gRPC::grpc++ protobuf::libprotobuf)
