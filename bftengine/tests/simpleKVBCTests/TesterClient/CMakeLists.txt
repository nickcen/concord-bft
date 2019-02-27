cmake_minimum_required (VERSION 3.2)
project(skvbc_client VERSION 0.1.0.0 LANGUAGES CXX)

add_compile_options(-std=c++11)

# GRPC and Protocol Buffers libraries location
list(APPEND CMAKE_PREFIX_PATH "/opt/grpc" "/opt/protobuf")

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

set(PROTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/../msgs/concord.proto
)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

set(client_sources
	main.cpp
	../simpleKVBCTests.cpp
)

add_executable(skvbc_client 
  ${client_sources}
  ${PROTO_SRCS}
  ${GRPC_SRCS}
)

target_link_libraries(skvbc_client LINK_PUBLIC simpleKVBC)

target_link_libraries(skvbc_client PUBLIC threshsign)

target_link_libraries(skvbc_client PUBLIC corebft)

target_link_libraries(skvbc_client PUBLIC gRPC::grpc++_reflection)

target_link_libraries(skvbc_client PUBLIC protobuf::libprotobuf)

target_include_directories(skvbc_client PUBLIC ../../SimpleKVBC/include)
target_include_directories(skvbc_client PUBLIC ..)

