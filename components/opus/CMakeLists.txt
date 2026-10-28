idf_component_register(
    INCLUDE_DIRS "include"
)

add_prebuilt_library(opus_lib
    "${CMAKE_CURRENT_LIST_DIR}/lib/libopus.a"
)

target_link_libraries(${COMPONENT_LIB} INTERFACE opus_lib)