
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# 2. Extract unique directories from all discovered source files
set(INCLUDE_DIRS "")
foreach(source_file ${SOURCES})
    get_filename_component(dir_path ${source_file} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${dir_path})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

# 3. Register the IDF component
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES 
      #  arduino-audio-tools
        esp_wifi
        fatfs
        nvs_flash
        esp_event
        esp_driver_i2s
        mbedtls#encryption
        app_update# this is for firmwire switch
        esp_http_server # for wifi privision
        esp_wifi_manager
        esp_bus  
        opus
)





