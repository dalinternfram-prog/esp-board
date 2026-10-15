# # 初始化源文件和包含目录
# set(SOURCES
#     "./main.c" 
#     "./hardeware_driver/bsp_board.c"
#     "./speech_det_driver/mic_speech.c"
#     "./audio_play_driver/audio_driver.c"
#     "./tca9555_driver/tca9555_driver.c"
#     "./button_driver/button_driver.c"
#     "./rgb_led_driver/rgb_led_driver.c"
#     "./Network/wifi_driver/wifi_manager.c"
# )

# set(INCLUDE_DIRS 
#     "."
#     "./hardeware_driver"
#     "./speech_det_driver"
#     "./audio_play_driver"
#     "./tca9555_driver"
#     "./button_driver"
#     "./rgb_led_driver"
#     "./Network/wifi_driver"
# )


# # 注册组件
# idf_component_register(
#     SRCS ${SOURCES}
#     INCLUDE_DIRS ${INCLUDE_DIRS}
#     REQUIRES arduino-audio-tools
# )





# 初始化源文件和包含目录
# set(SOURCES
#     "./main.c" 
#     "./hardeware_driver/bsp_board.c"
#     "./speech_det_driver/mic_speech.c"
#     "./audio_play_driver/audio_driver.c"
#     "./tca9555_driver/tca9555_driver.c"
#     "./button_driver/button_driver.c"
#     "./rgb_led_driver/rgb_led_driver.c"
#     "./Network/wifi_driver/wifi_manager.c"
# )

# set(INCLUDE_DIRS 
#     "."
#     "./hardeware_driver"
#     "./speech_det_driver"
#     "./audio_play_driver"
#     "./tca9555_driver"
#     "./button_driver"
#     "./rgb_led_driver"
#     "./Network/wifi_driver"
# )

# 初始化源文件和包含目录
set(SOURCES
    "./main.c" 
    "./macroPhone/speech_engine/speech_engine.cpp"
    "./macroPhone/audio_processor/audio_processor.cpp"
    "./macroPhone/Hardware Audio Capture Driver/audio_capture_driver.cpp"
    "./hardeware_driver/bsp_board.c"
    "./speech_det_driver/mic_speech.c"
    "./audio_play_driver/audio_driver.c"
    "./tca9555_driver/tca9555_driver.c"
    "./button_driver/button_driver.c"
    "./rgb_led_driver/rgb_led_driver.c"
    "./Network/wifi_driver/wifi_manager.c"
       "./audio/AudioStreamer.cpp"
)

set(INCLUDE_DIRS 
    "."
    "./macroPhone/speech_engine"
    "./macroPhone/audio_processor"
    "./macroPhone/Hardware Audio Capture Driver"
    "./audio"
    "./hardeware_driver"
    "./speech_det_driver"
    "./audio_play_driver"
    "./tca9555_driver"
    "./button_driver"
    "./rgb_led_driver"
    "./Network/wifi_driver"
)
# 注册组件
# idf_component_register(
#     SRCS ${SOURCES}
#     INCLUDE_DIRS ${INCLUDE_DIRS}
#     REQUIRES arduino-audio-tools
# )


idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES 
        arduino-audio-tools
        esp_wifi
        fatfs
        nvs_flash
        esp_event
        esp_driver_i2s
)







