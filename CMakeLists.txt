# cmake_minimum_required(VERSION 3.5)



# include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# # Apply RTTI and exceptions ONLY to C++ files globally
# idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-frtti>" APPEND)
# idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-fexceptions>" APP

# project(factory_01)
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# components/proto is a symlink to the shared audio-server/proto tree. Its
# CMakeLists.txt only takes the ESP-IDF branch (idf_component_register) when
# PROTO_BUILD_SERVER_LIB is set, which audio-server/src/CMakeLists.txt does for
# the host build. Define it here for the firmware build; otherwise the file
# skips registration and falls through to the host-only
# add_library(ProtocolShared INTERFACE) block.
set(PROTO_BUILD_SERVER_LIB ON)
# ESP-IDF v6.x collects component REQUIREMENTS by re-running each component's
# CMakeLists.txt in a separate `cmake -P` process
# (tools/cmake/scripts/component_get_requirements.cmake). That process does not
# inherit normal CMake variables, only the build properties serialized to
# build_properties.temp.cmake, so export the flag as a build property too.
# Without it, the script-mode pass reaches add_library() and fails with
# "add_library command is not scriptable".
idf_build_set_property(PROTO_BUILD_SERVER_LIB ON)

project(factory_01)

# Apply RTTI and exceptions ONLY to C++ files globally
idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-frtti>" APPEND)
idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-fexceptions>" APPEND)