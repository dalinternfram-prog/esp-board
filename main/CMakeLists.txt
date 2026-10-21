

# set(SOURCES
#     "./main.c" 
#     "./macroPhone/speech_engine/speech_engine.cpp"
#     "./macroPhone/audio_processor/audio_processor.cpp"
#     "./macroPhone/Hardware Audio Capture Driver/audio_capture_driver.cpp"
#     "./hardeware_driver/bsp_board.c"
#     "./speech_det_driver/mic_speech.c"
#     "./audio_play_driver/audio_driver.c"
#     "./tca9555_driver/tca9555_driver.c"
#     "./button_driver/button_driver.c"
#     "./rgb_led_driver/rgb_led_driver.c"
#     "./Network/wifi_driver/wifi_manager.c"
#         "./Network/kcp/ikcp.c"
#        "./audio/AudioStreamer.cpp"
# )

# set(INCLUDE_DIRS 
#     "."
#     "./macroPhone/speech_engine"
#     "./macroPhone/audio_processor"
#     "./macroPhone/Hardware Audio Capture Driver"
#     "./audio"
#     "./hardeware_driver"
#     "./speech_det_driver"
#     "./audio_play_driver"
#     "./tca9555_driver"
#     "./button_driver"
#     "./rgb_led_driver"
#     "./Network/wifi_driver"
#       "./Network/kcp"
# )



# idf_component_register(
#     SRCS ${SOURCES}
#     INCLUDE_DIRS ${INCLUDE_DIRS}
#     REQUIRES 
#         arduino-audio-tools
#         esp_wifi
#         fatfs
#         nvs_flash
#         esp_event
#         esp_driver_i2s
# )

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
        libopus
)





