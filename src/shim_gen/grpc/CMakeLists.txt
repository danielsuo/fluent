CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

ADD_EXECUTABLE(shim_gen_grpc_fluent_plugin
    cpp_generator.cc
    fluent_plugin.cc)

TARGET_LINK_LIBRARIES(shim_gen_grpc_fluent_plugin
    fmt
    glog
    protobuf
    protoc
    pthread)

ADD_DEPENDENCIES(shim_gen_grpc_fluent_plugin
    ${FMT_PROJECT}
    ${GOOGLELOG_PROJECT}
    ${GRPC_PROJECT})
