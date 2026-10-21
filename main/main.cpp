  #define MainRun
  
  #if defined (MainRun)
#include "Opus.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h" 
#include "bsp_board.h"
#include "tca9555_driver.h"
#include "mic_speech.h"
#include "audio_driver.h"
#include "rgb_led_driver.h"
#include "button_driver.h"  
#include "macroPhone/arguments.hpp" 
#include "nvs_manager.hpp"
#include <memory>
#include "file_manager.hpp"
#include "firmware_switch.hpp"
#include "wifi/shared.hpp"
#include "Net manager.hpp"
// CHANGED: Added const to fix "ISO C++ forbids converting a string constant to 'char*'" warning
static const char *TAG = "app main";

#define MAX_MP3_FILE    50
static char SD_Name[MAX_MP3_FILE][100]; 
static uint16_t Search_mp3_file_count = 0;
static char music_buf[120] = {0};
static int16_t now_play_id = 0;

void app_run_loop( 
     std::shared_ptr<NvsManager> nvsManager, 
     std::shared_ptr<FirmwareSwitch> firmwareSwitch
);
// 按键回调函数
static void on_key_event(key_id_t id, key_event_t event, void *user_data) 
{
    int vol;
    switch (id) 
    {
        case KEY_ID_9:
            if (event == KEY_EVENT_SHORT_PRESS) 
            {
                vol = get_audio_volume();
                vol+=10;
                if(vol>Volume_MAX)
                    vol = Volume_MAX;
                Volume_Adjustment(vol);
                ESP_LOGI(TAG, "vol=%d",vol);
            } 
            else 
            {
                Audio_Stop_Play();
                if(Search_mp3_file_count==0)
                    return;
                now_play_id--;
                if(now_play_id<0)
                {
                    now_play_id = Search_mp3_file_count-1;
                }
                memset(music_buf,0,sizeof(music_buf));
                sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                Audio_Play_Music(music_buf);
                ESP_LOGI(TAG, "上一首");
            }
            break;
        case KEY_ID_10:
            if (event == KEY_EVENT_SHORT_PRESS) 
            {
                esp_asp_state_t state =  Audio_Get_Current_State();
                switch (state)
                {
                    case ESP_ASP_STATE_NONE:
                        if(Search_mp3_file_count==0)
                            return;
                        memset(music_buf,0,sizeof(music_buf));
                        sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                        Audio_Play_Music(music_buf);
                        set_rgb_mode(RGB_MODE_PLAYING);
                        ESP_LOGI(TAG, "播放:%s",music_buf);
                        break;
                    case ESP_ASP_STATE_RUNNING:
                        ESP_LOGI(TAG, "暂停播放");
                        Audio_Pause_Play();
                        set_rgb_mode(RGB_MODE_IDLE);
                        break;
                    case ESP_ASP_STATE_PAUSED:
                        ESP_LOGI(TAG, "恢复播放");
                        Audio_Resume_Play();
                        set_rgb_mode(RGB_MODE_PLAYING);
                        break;
                    case ESP_ASP_STATE_STOPPED:
                        memset(music_buf,0,sizeof(music_buf));
                        sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                        Audio_Play_Music(music_buf);
                        ESP_LOGI(TAG, "播放:%s",music_buf);
                        set_rgb_mode(RGB_MODE_PLAYING);
                        break;
                    case ESP_ASP_STATE_FINISHED:
                        ESP_LOGI(TAG, "播放完毕");
                        set_rgb_mode(RGB_MODE_IDLE);
                        break;
                    default:
                        break;
                }
                
            } 
            else 
            {
                Audio_Stop_Play();
                ESP_LOGI(TAG, "停止播放");
            }
            break;
        case KEY_ID_11:
            if (event == KEY_EVENT_SHORT_PRESS) 
            {
                vol = get_audio_volume();
                vol-=10;
                if(vol<0)
                    vol = 0;
                Volume_Adjustment(vol);
                ESP_LOGI(TAG, "vol=%d",vol);
            } 
            else 
            {
                Audio_Stop_Play();
                if(Search_mp3_file_count==0)
                    return;
                now_play_id++;
                if(now_play_id==Search_mp3_file_count)
                {
                    now_play_id = 0;
                }
                memset(music_buf,0,sizeof(music_buf));
                sprintf(music_buf,"file://sdcard/%s",SD_Name[now_play_id]);
                Audio_Play_Music(music_buf);
                ESP_LOGI(TAG, "上一首");
            }
            break;
    }
}

//语音唤醒和命令词回调函数
static void Speech_event_callback(esp_sr_rec_event_t event,esp_sr_evt_data_t evt_data, void *user_data)
{
    static bool play_flag = 0;
    switch (event)
    {
        // CHANGED: Enclosed case block in {} scope to fix cross-initialization compile errors in C++
        case ESP_SR_EVT_AWAKEN: {
            ESP_LOGI(TAG, "ESP_SR_EVT_AWAKEN = %d",evt_data.awaken_channel);
            esp_asp_state_t state =  Audio_Get_Current_State();
            if(state==ESP_ASP_STATE_RUNNING)
            {
                ESP_LOGI(TAG, "暂停播放");
                Audio_Pause_Play();
                play_flag = 1;
            }
            set_rgb_mode(RGB_MODE_REC_COMMAND);
            break;
        }
        // CHANGED: Enclosed case block in {} scope
        case ESP_SR_EVT_CMD: {
            //ESP_LOGI(TAG, "ESP_SR_EVT_CMD = %d",evt_data.sr_cmd);
            switch (evt_data.sr_cmd)
            {
                case 0:
                    ESP_LOGI(TAG, "灯光变成红色");
                    set_rgb_color(RGB_COLOR_RED);
                    break;
                case 1:
                    ESP_LOGI(TAG, "灯光变成蓝色");
                    set_rgb_color(RGB_COLOR_BLUE);
                    break;
                case 2:
                    ESP_LOGI(TAG, "灯光变成绿色");
                    set_rgb_color(RGB_COLOR_GREEN);
                    break;
                case 3:
                    ESP_LOGI(TAG, "灯光变成白色");
                    set_rgb_color(RGB_COLOR_WHITE);
                    break;
                default:
                    break;
            }
            break;
        }
        // CHANGED: Enclosed case block in {} scope
        case ESP_SR_EVT_CMD_TIMEOUT: {
            ESP_LOGI(TAG, "ESP_SR_EVT_CMD_TIMEOUT");

            if(play_flag == 1)
            {
                play_flag = 0;
                ESP_LOGI(TAG, "恢复播放");
                Audio_Resume_Play();
                set_rgb_mode(RGB_MODE_PLAYING);
            }
            else
            {
                set_rgb_mode(RGB_MODE_IDLE);
            }
            break;
        }
        default:
            break;
    }
}

//扫描TF卡MP3文件
static void Search_mp3_Music(void)     
{        
    // CHANGED: Cast string literal to char* to fix C++ string conversion warning
    Search_mp3_file_count = Folder_retrieval(const_cast<char*>("/sdcard"), const_cast<char*>(".mp3"), SD_Name, MAX_MP3_FILE);
    printf("file_count=%d\r\n",Search_mp3_file_count);
    if(Search_mp3_file_count) 
    {  
        for (int i = 0; i < Search_mp3_file_count; i++) 
        {
            ESP_LOGI("SAFASF","%s",SD_Name[i]);
        }                
        
    }                                                               
}

extern "C" void app_main()
{ 

            ESP_LOGI("++++++++++++","Hello,can you hear me???" );
    ESP_ERROR_CHECK(esp_board_init(macroPhone::argument::sample_rate, macroPhone::argument::channel_count, macroPhone::argument::bits_per_chan));

            ESP_LOGI("++++++++++++","success esp_board_init" );
   tca9555_driver_init();

            ESP_LOGI("++++++++++++","success tca9555_driver_init" );
   // CHANGED: Cast string literal to char* to fix C++ string conversion warning
   esp_sdcard_init(const_cast<char*>("/sdcard"), 10);
    
            ESP_LOGI("++++++++++++","success esp_sdcard_init" );
   //Speech_Init();

            ESP_LOGI("++++++++++++","success Speech_Init" );
   Speech_register_callback(Speech_event_callback);

            ESP_LOGI("++++++++++++","success Speech_register_callback" );
    Audio_Play_Init();

            ESP_LOGI("++++++++++++","success Audio_Play_Init" );
     RGB_Example();

            ESP_LOGI("++++++++++++","success RGB_Example" );
     key_module_init(NULL);

            ESP_LOGI("++++++++++++","success key_module_init" );
     key_register_callback(on_key_event);
    
            ESP_LOGI("++++++++++++","success key_register_callback" );
     Search_mp3_Music();

            ESP_LOGI("++++++++++++","success Search_mp3_Music" );
 

std::shared_ptr<NvsManager> NvsManager_class=std::make_shared<NvsManager>();
 

std::shared_ptr<FirmwareSwitch> FirmwareSwitch_class=std::make_shared<FirmwareSwitch>();


NvsManager_class->init(); 
FirmwareSwitch_class->init();

FileManager::init();


wifi::init();

Network::init(222);
 app_run_loop( NvsManager_class, FirmwareSwitch_class);

   // xTaskCreate(opus_class_test_task, "opus_class_test_task", 32768, NULL, 5, NULL);
}

#endif
#if defined (LC3Test)

 #include <stdio.h>
#include <vector>
#include <cmath>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include the LC3 API safely in C++
extern "C" {
    #include "lc3.h"
}

static const char* TAG = "LC3_TEST";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting LC3 codec test...");

    // 1. Configuration parameters
    int sample_rate = 16000;       // 16 kHz sample rate
    int frame_duration_us = 10000; // 10 ms = 10000 microseconds
    
    // Calculate samples per frame: (16000 * 0.010) = 160 samples
    int samples_per_frame = (sample_rate * (frame_duration_us / 1000)) / 1000;

    // 2. Initialize Encoder memory (uses microseconds!)
    int enc_size = lc3_encoder_size(frame_duration_us, sample_rate);
    std::vector<uint8_t> enc_mem(enc_size);
    // Parameters: frame_us, hertz, epmr (0), buffer memory
    lc3_encoder_t encoder = lc3_setup_encoder(frame_duration_us, sample_rate, 0, enc_mem.data());

    if (!encoder) {
        ESP_LOGE(TAG, "Failed to setup LC3 encoder!");
        return;
    }
    ESP_LOGI(TAG, "Encoder setup successful!");

    // 3. Initialize Decoder memory
    int dec_size = lc3_decoder_size(frame_duration_us, sample_rate);
    std::vector<uint8_t> dec_mem(dec_size);
lc3_decoder_t decoder = lc3_setup_decoder(frame_duration_us, sample_rate, 0, dec_mem.data());

    if (!decoder) {
        ESP_LOGE(TAG, "Failed to setup LC3 decoder!");
        return;
    }
    ESP_LOGI(TAG, "Decoder setup successful!");

    // 4. Generate hardcoded PCM input (a tiny 440 Hz tone sine wave for 10ms)
    std::vector<int16_t> pcm_in(samples_per_frame);
    for (int i = 0; i < samples_per_frame; i++) {
        float t = (float)i / sample_rate;
        pcm_in[i] = (int16_t)(10000.0f * sinf(2.0f * M_PI * 440.0f * t));
    }

    // 5. Allocate output bitstream buffer (LC3 compressed data)
    std::vector<uint8_t> bitstream(400);
    int target_num_bytes = 40; // Target compressed size in bytes

    // 6. Test Encoding
    ESP_LOGI(TAG, "Encoding %d PCM samples into %d bytes...", samples_per_frame, target_num_bytes);
    int err = lc3_encode(encoder, LC3_PCM_FORMAT_S16, pcm_in.data(), 1, target_num_bytes, bitstream.data());
    
    if (err != 0) {
        ESP_LOGE(TAG, "lc3_encode failed with error code: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Encoding successful!");

    // 7. Test Decoding back to PCM
    std::vector<int16_t> pcm_out(samples_per_frame);
    ESP_LOGI(TAG, "Decoding bitstream back to PCM...");
    
    err = lc3_decode(decoder, bitstream.data(), target_num_bytes, LC3_PCM_FORMAT_S16, pcm_out.data(), 1);
    
    if (err != 0) {
        ESP_LOGE(TAG, "lc3_decode failed with error code: %d", err);
        return;
    }

    ESP_LOGI(TAG, "Decoding successful! First 5 decoded samples: %d, %d, %d, %d, %d",
             pcm_out[0], pcm_out[1], pcm_out[2], pcm_out[3], pcm_out[4]);

    ESP_LOGI(TAG, "LC3 pipeline test passed completely!");
}

#endif




#if defined (OpusTest)



 #include <stdio.h>
#include <vector>
#include <cmath>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
    #include "esp_opus_dec.h"
}

static const char* TAG = "OPUS_TEST";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting Opus decoder test...");

    esp_opus_dec_cfg_t cfg = ESP_OPUS_DEC_CONFIG_DEFAULT();
    cfg.sample_rate = ESP_AUDIO_SAMPLE_RATE_16K;
    cfg.channel = ESP_AUDIO_MONO;
    cfg.frame_duration = ESP_OPUS_DEC_FRAME_DURATION_10_MS;

    void* dec_handle = nullptr;
    esp_audio_err_t err = esp_opus_dec_open(&cfg, sizeof(cfg), &dec_handle);
    if (err != ESP_AUDIO_ERR_OK || !dec_handle) {
        ESP_LOGE(TAG, "esp_opus_dec_open failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Opus decoder opened successfully!");

    std::vector<uint8_t> dummy_bitstream(40, 0);
    std::vector<int16_t> pcm_out(960);

    esp_audio_dec_in_raw_t raw = {
        .buffer = dummy_bitstream.data(),
        .len = (uint32_t)dummy_bitstream.size()
    };

    esp_audio_dec_out_frame_t frame = {
        .buffer = (uint8_t*)pcm_out.data(),
        .len = (uint32_t)(pcm_out.size() * sizeof(int16_t))
    };

    esp_audio_dec_info_t info = {};

    err = esp_opus_dec_decode(dec_handle, &raw, &frame, &info);
    if (err != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "esp_opus_dec_decode failed: %d", err);
    } else {
        ESP_LOGI(TAG, "Opus decode API executed successfully! Output bytes: %lu", (unsigned long)frame.len);
    }

    esp_opus_dec_close(dec_handle);
    ESP_LOGI(TAG, "Opus test complete!");
}
#endif






#if defined (OpusClassTest)
 #include <stdio.h>
#include <vector>
#include <cmath>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Opus.hpp" // Include your SimpleOpusEncoder class header

static const char* TAG = "OPUS_ENCODER_CLASS_TEST";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting SimpleOpusEncoder class test...");

    // 1. Configure the encoder settings
    SimpleOpusEncoder::Config config;
    config.sample_rate = 16000;         // 16 kHz sample rate
    config.channels = 1;                // Mono channel
    config.frame_duration_ms = 10;      // 10 ms frame duration

    // 2. Instantiate the wrapper object
    SimpleOpusEncoder encoder(config);

    // 3. Initialize the encoder instance
    if (encoder.init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SimpleOpusEncoder!");
        return;
    }
    ESP_LOGI(TAG, "SimpleOpusEncoder initialized successfully!");

    // 4. Calculate expected samples per channel based on sample rate and frame duration
    // (16000 Hz * 0.010 s) = 160 samples per channel
    size_t samples_per_channel = (config.sample_rate * config.frame_duration_ms) / 1000;
    size_t total_samples = samples_per_channel * config.channels;

    // 5. Generate hardcoded PCM input (a 440 Hz sine wave for 10ms)
    std::vector<int16_t> pcm_in(total_samples);
    for (size_t i = 0; i < total_samples; i++) {
        float t = (float)i / config.sample_rate;
        pcm_in[i] = (int16_t)(10000.0f * sinf(2.0f * M_PI * 440.0f * t));
    }

    // 6. Test vector-based encode method (passing maximum output size buffer)
    ESP_LOGI(TAG, "Encoding %zu PCM samples...", pcm_in.size());
    std::vector<uint8_t> compressed_data = encoder.encode(pcm_in, 1512);

    if (compressed_data.empty()) {
        ESP_LOGE(TAG, "Encoding failed or returned empty payload!");
    } else {
        ESP_LOGI(TAG, "Encoding successful! Compressed payload size: %zu bytes", compressed_data.size());
        
        // Print first few bytes of the compressed Opus bitstream payload
        ESP_LOGI(TAG, "First 5 compressed bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X",
                 compressed_data[0],
                 compressed_data.size() > 1 ? compressed_data[1] : 0,
                 compressed_data.size() > 2 ? compressed_data[2] : 0,
                 compressed_data.size() > 3 ? compressed_data[3] : 0,
                 compressed_data.size() > 4 ? compressed_data[4] : 0);
    }

    // 7. Clean up (the destructor will also automatically invoke deinit() when going out of scope)
    encoder.deinit();
    ESP_LOGI(TAG, "SimpleOpusEncoder class test completed cleanly!");
}
#endif






#if defined (OpusEncodeTestSuccess)

#include <stdio.h>
#include <vector>
#include <cmath>
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
    #include "esp_audio_enc.h"
    #include "esp_audio_enc_default.h"
    #include "esp_opus_enc.h"
}

static const char* TAG = "OPUS_ENC_TEST";

static void opus_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting Opus test with massive 32KB stack...");

    esp_audio_enc_register_default();

    static esp_opus_enc_config_t opus_cfg = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .bitrate = 24000,
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_10_MS,
        .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,
        .complexity = 3,
        .enable_fec = false,
        .enable_dtx = false,
        .enable_vbr = true
    };

    esp_audio_enc_config_t cfg = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(opus_cfg)
    };

    esp_audio_enc_handle_t enc_handle = NULL;
    if (esp_audio_enc_open(&cfg, &enc_handle) != ESP_AUDIO_ERR_OK || !enc_handle) {
        ESP_LOGE(TAG, "Open failed");
        vTaskDelete(NULL);
        return;
    }

    int in_size = 0, out_size = 0;
    esp_audio_enc_get_frame_size(enc_handle, &in_size, &out_size);

    int samples = in_size / sizeof(int16_t);
    std::vector<int16_t> pcm_in(samples);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / 16000.0f;
        pcm_in[i] = (int16_t)(8000.0f * sinf(2.0f * 3.14159f * 440.0f * t));
    }

    std::vector<uint8_t> bitstream(out_size, 0);

    esp_audio_enc_in_frame_t in_frame = {};
    in_frame.buffer = (uint8_t*)pcm_in.data();
    in_frame.len = (uint32_t)in_size;

    esp_audio_enc_out_frame_t out_frame = {};
    out_frame.buffer = bitstream.data();
    out_frame.len = (uint32_t)out_size;

    ESP_LOGI(TAG, "Executing encoding process...");
    if (esp_audio_enc_process(enc_handle, &in_frame, &out_frame) == ESP_AUDIO_ERR_OK) {
        ESP_LOGI(TAG, "SUCCESS! Encoded bytes: %lu", (unsigned long)out_frame.encoded_bytes);
    } else {
        ESP_LOGE(TAG, "Encoding process failed");
    }

    ESP_LOGI(TAG, "Test completed cleanly!");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started...");
    // Bumped stack size to 32768 (32 KB) to accommodate Opus's heavy internal stack usage
    xTaskCreate(opus_test_task, "opus_test_task", 32768, NULL, 5, NULL);
}
#endif



 #if defined (OpusEncodeTest)

#include <stdio.h>
#include <vector>
#include <cmath>
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
    #include "esp_audio_enc.h"
    #include "esp_audio_enc_default.h"
    #include "esp_opus_enc.h"
}

static const char* TAG = "OPUS_ENC_TEST";

static void opus_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting Opus test with static configuration and correct close syntax...");

    esp_audio_enc_register_default();

    static esp_opus_enc_config_t opus_cfg = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .bitrate = 24000,
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_10_MS,
        .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,
        .complexity = 3,
        .enable_fec = false,
        .enable_dtx = false,
        .enable_vbr = true
    };

    static esp_audio_enc_config_t cfg = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(opus_cfg)
    };

    esp_audio_enc_handle_t enc_handle = NULL;
    if (esp_audio_enc_open(&cfg, &enc_handle) != ESP_AUDIO_ERR_OK || !enc_handle) {
        ESP_LOGE(TAG, "Open failed");
        vTaskDelete(NULL);
        return;
    }

    int in_size = 0, out_size = 0;
    esp_audio_enc_get_frame_size(enc_handle, &in_size, &out_size);

    int samples = in_size / sizeof(int16_t);
    std::vector<int16_t> pcm_in(samples);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / 16000.0f;
        pcm_in[i] = (int16_t)(8000.0f * sinf(2.0f * 3.14159f * 440.0f * t));
    }

    std::vector<uint8_t> bitstream(out_size, 0);

    esp_audio_enc_in_frame_t in_frame = {};
    in_frame.buffer = (uint8_t*)pcm_in.data();
    in_frame.len = (uint32_t)in_size;

    esp_audio_enc_out_frame_t out_frame = {};
    out_frame.buffer = bitstream.data();
    out_frame.len = (uint32_t)out_size;

    ESP_LOGI(TAG, "Executing encoding process...");
    if (esp_audio_enc_process(enc_handle, &in_frame, &out_frame) == ESP_AUDIO_ERR_OK) {
        ESP_LOGI(TAG, "SUCCESS! Encoded bytes: %lu", (unsigned long)out_frame.encoded_bytes);
    } else {
        ESP_LOGE(TAG, "Encoding process failed");
    }

    // Call close as a void function now that config pointers are static and stable
    ESP_LOGI(TAG, "Closing encoder instance...");
    esp_audio_enc_close(enc_handle);
    ESP_LOGI(TAG, "Encoder closed successfully!");

    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started...");
    xTaskCreate(opus_test_task, "opus_test_task", 32768, NULL, 5, NULL);
}
#endif








#if defined (OpusEncodeClassTest) 

#include <stdio.h>
#include <vector>
#include <cmath>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include your custom C++ class header
#include "Opus.hpp"

extern "C" {
    #include "esp_audio_enc_default.h" // Required to register backend codecs into the registry framework
}

static const char* TAG = "OPUS_CLASS_TEST";

static void opus_class_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting SimpleOpusEncoder class test task...");

    // 1. CRITICAL: Register default audio codecs into Espressif's common registry table
    esp_audio_err_t reg_err = esp_audio_enc_register_default();
    if (reg_err != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register default audio encoders: 0x%x", reg_err);
        vTaskDelete(NULL);
        return;
    }

    // 2. Configure the encoder settings matching your struct fields
    SimpleOpusEncoder::Config config;
    config.sample_rate = 16000;
    config.channels = 1;
    config.frame_duration_ms = 10; // 10ms frame duration

    // 3. Instantiate your wrapper class
    SimpleOpusEncoder encoder(config);

    // 4. Initialize the encoder (this will open the handle and query frame sizes)
    if (encoder.init() != ESP_OK) {
        ESP_LOGE(TAG, "SimpleOpusEncoder initialization failed!");
        vTaskDelete(NULL);
        return;
    }

    // 5. Calculate exact sample count expected by your class validation check:
    // formula used in your class: (sample_rate * frame_duration_ms) / 1000 * channels
    size_t expected_samples = (config.sample_rate * config.frame_duration_ms) / 1000 * config.channels;
    
    ESP_LOGI(TAG, "Generating %zu PCM samples for encoding...", expected_samples);
    std::vector<int16_t> pcm_samples(expected_samples);
    
    // Fill with a real 440Hz sine wave signal
    for (size_t i = 0; i < expected_samples; i++) {
        float t = (float)i / (float)config.sample_rate;
        pcm_samples[i] = (int16_t)(8000.0f * sinf(2.0f * 3.14159f * 440.0f * t));
    }

    // 6. Test encoding using the vector overload method
    // Provide a safe maximum output buffer size (e.g., 512 bytes)
    uint32_t max_out_size = 512;
    ESP_LOGI(TAG, "Calling encoder.encode(...) vector method...");
    
    std::vector<uint8_t> compressed_data = encoder.encode(pcm_samples, max_out_size);

    if (!compressed_data.empty()) {
        ESP_LOGI(TAG, "SUCCESS! Encoded Opus packet size: %u bytes", (unsigned int)compressed_data.size());
    } else {
        ESP_LOGE(TAG, "Encoding failed or returned empty payload.");
    }

    // 7. Clean up (destructor will also automatically call deinit when encoder goes out of scope)
    encoder.deinit();
    ESP_LOGI(TAG, "Test task finished cleanly.");

    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started. Spawning encoder class test task with 32KB stack...");
    
    // Spin up in a dedicated FreeRTOS task with 32KB stack to accommodate Opus stack consumption
    xTaskCreate(opus_class_test_task, "opus_class_test_task", 32768, NULL, 5, NULL);
}
 
#endif