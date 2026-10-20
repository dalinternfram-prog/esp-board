# cmake_minimum_required(VERSION 3.5)



# include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# # Apply RTTI and exceptions ONLY to C++ files globally
# idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-frtti>" APPEND)
# idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-fexceptions>" APP

# project(factory_01)
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(factory_01)

# Apply RTTI and exceptions ONLY to C++ files globally
idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-frtti>" APPEND)
idf_build_set_property(COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CXX>:-fexceptions>" APPEND)