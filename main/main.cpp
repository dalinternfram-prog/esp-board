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

// CHANGED: Added const to fix "ISO C++ forbids converting a string constant to 'char*'" warning
static const char *TAG = "app main";

#define MAX_MP3_FILE    50
static char SD_Name[MAX_MP3_FILE][100]; 
static uint16_t Search_mp3_file_count = 0;
static char music_buf[120] = {0};
static int16_t now_play_id = 0;
//TODO change 32768 To 49152
static int magicStackNumber = 9152;
 
void app_run_loop(void* pvParameters);
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

    //         ESP_LOGI("++++++++++++","success Audio_Play_Init" );
    //  RGB_Example();

            ESP_LOGI("++++++++++++","success RGB_Example" );
     key_module_init(NULL);

            ESP_LOGI("++++++++++++","success key_module_init" );
     key_register_callback(on_key_event);
    
            ESP_LOGI("++++++++++++","success key_register_callback" );
     Search_mp3_Music();

            ESP_LOGI("++++++++++++","success Search_mp3_Music" );
 
 ESP_LOGI("MAIN", "Initializing system from app_main...");

    // Create the application loop task with a safe stack size (e.g., 8192 bytes) and priority 5
    xTaskCreate(
        app_run_loop,          // Task function
        "app_run_loop_task",   // Task name for debugging
        magicStackNumber,                  // Stack depth (bytes)
        NULL,                  // Parameters to pass
        5,                     // Task priority
        NULL                   // Task handle output
    );

    ESP_LOGI("MAIN", "app_main returning; application task running in background.");
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




#if defined (OpusDecodeTestPassed)



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






#if defined (OpusClassDecodeTest)
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








#if defined (OpusEncodeClassTest1) 

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


#if defined (OpusEncodeClassTestLoopPassed) 

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
    ESP_LOGI(TAG, "Starting continuous SimpleOpusEncoder class test task...");

    // 1. Register default audio codecs into Espressif's common registry table
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

    // 4. Initialize the encoder (opens handle and queries frame sizes)
    if (encoder.init() != ESP_OK) {
        ESP_LOGE(TAG, "SimpleOpusEncoder initialization failed!");
        vTaskDelete(NULL);
        return;
    }

    // 5. Calculate exact sample count expected per frame:
    // formula: (sample_rate * frame_duration_ms) / 1000 * channels
    size_t expected_samples = (config.sample_rate * config.frame_duration_ms) / 1000 * config.channels;
    
    ESP_LOGI(TAG, "Preparing PCM frame buffer of size: %zu samples", expected_samples);
    std::vector<int16_t> pcm_samples(expected_samples);
    
    uint32_t max_out_size = 512;
    uint32_t frame_counter = 0;

    // 6. Continuous loop test
    ESP_LOGI(TAG, "Entering continuous encoding loop...");
    while (true) {
        // Fill frame with a rolling 440Hz sine wave signal (simulating real live audio input)
        for (size_t i = 0; i < expected_samples; i++) {
            float t = (float)(frame_counter * expected_samples + i) / (float)config.sample_rate;
            pcm_samples[i] = (int16_t)(8000.0f * sinf(2.0f * 3.14159f * 440.0f * t));
        }

        // Call the vector overload method to encode the PCM block
        std::vector<uint8_t> compressed_data = encoder.encode(pcm_samples, max_out_size);

        if (!compressed_data.empty()) {
          
                ESP_LOGI(TAG, "Frame [%u] SUCCESS! Encoded Opus packet size: %u bytes", 
                         (unsigned int)frame_counter, (unsigned int)compressed_data.size());
            
        } else {
            ESP_LOGE(TAG, "Frame [%u] Encoding failed or returned empty payload.", (unsigned int)frame_counter);
        }

        frame_counter++;

        // Yield to the FreeRTOS kernel and keep the Task Watchdog Timer (WDT) happy
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }

    // (Unreachable cleanup code, kept for structural completeness if loop ever breaks)
    encoder.deinit();
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started. Spawning continuous encoder test task with 32KB stack...");
    
    // Spin up in a dedicated FreeRTOS task with 32KB stack to accommodate Opus heap/stack requirements
    xTaskCreate(opus_class_test_task, "opus_class_test_task", 32768, NULL, 5, NULL);
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
    ESP_LOGI(TAG, "Starting continuous SimpleOpusEncoder class test task...");

    // 1. Register default audio codecs into Espressif's common registry table
    esp_audio_err_t reg_err = esp_audio_enc_register_default();
    if (reg_err != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register default audio encoders: 0x%x", reg_err);
        vTaskDelete(NULL);
        return;
    }

    // 2. Configure the encoder settings matching your struct fields
    SimpleOpusEncoder::Config config;
    config.sample_rate = 16000;
    config.channels = 2;
    config.frame_duration_ms = 20; // 10ms frame duration

    // 3. Instantiate your wrapper class
    SimpleOpusEncoder encoder(config);

    // 4. Initialize the encoder (opens handle and queries frame sizes)
    if (encoder.init() != ESP_OK) {
        ESP_LOGE(TAG, "SimpleOpusEncoder initialization failed!");
        vTaskDelete(NULL);
        return;
    }

    // 5. Calculate exact sample count expected per frame:
    // formula: (sample_rate * frame_duration_ms) / 1000 * channels
    size_t expected_samples = (config.sample_rate * config.frame_duration_ms) / 1000 * config.channels;
    
    ESP_LOGI(TAG, "Preparing PCM frame buffer of size: %zu samples", expected_samples);
    std::vector<int16_t> pcm_samples(expected_samples);
    
    uint32_t max_out_size = 512;
    uint32_t frame_counter = 0;

    // 6. Continuous loop test
    ESP_LOGI(TAG, "Entering continuous encoding loop...");
    while (true) {
        // Fill frame with a rolling 440Hz sine wave signal (simulating real live audio input)
        for (size_t i = 0; i < expected_samples; i++) {
            float t = (float)(frame_counter * expected_samples + i) / (float)config.sample_rate;
            pcm_samples[i] = (int16_t)(8000.0f * sinf(2.0f * 3.14159f * 440.0f * t));
        }

        // Call the vector overload method to encode the PCM block
        std::vector<uint8_t> compressed_data = encoder.encode(pcm_samples, max_out_size);

        if (!compressed_data.empty()) {
          
                ESP_LOGI(TAG, "Frame [%u] SUCCESS! Encoded Opus packet size: %u bytes", 
                         (unsigned int)frame_counter, (unsigned int)compressed_data.size());
            
        } else {
            ESP_LOGE(TAG, "Frame [%u] Encoding failed or returned empty payload.", (unsigned int)frame_counter);
        }

        frame_counter++;

        // Yield to the FreeRTOS kernel and keep the Task Watchdog Timer (WDT) happy
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }

    // (Unreachable cleanup code, kept for structural completeness if loop ever breaks)
    encoder.deinit();
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started. Spawning continuous encoder test task with 32KB stack...");
    
    // Spin up in a dedicated FreeRTOS task with 32KB stack to accommodate Opus heap/stack requirements
    xTaskCreate(opus_class_test_task, "opus_class_test_task", 32768, NULL, 5, NULL);
}

#endif

























 #if defined (FileVectorTest1)

#include <stdio.h>
#include <vector>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "file-vector.hpp"
#include "file_manager.hpp" // For file deletion helpers

static const char* TAG = "FILE_VECTOR_TEST";

static void file_vector_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting fileVector API test task...");

    // 1. Initialize storage
    FileManager::init();

    std::string base_name = "opus_test_storage";
    std::string bin_file = base_name;
    std::string json_file = base_name + ".json";

    // CLEAN UP OLD RUNS: Delete leftover files so we start fresh every boot
    if (FileManager::exists(bin_file))  FileManager::deleteFile(bin_file);
    if (FileManager::exists(json_file)) FileManager::deleteFile(json_file);

    // 2. Instantiate fileVector
    file::fileVector fv(base_name);

    // 3. Prepare mock byte packets (making sure we know their exact sizes)
    std::vector<uint8_t> packet1 = {0x01, 0x02, 0x03, 0x04, 0x05}; // 5 bytes
    std::vector<uint8_t> packet2 = {0x0A, 0x0B, 0x0C, 0x0D};        // 4 bytes
    std::vector<uint8_t> packet3 = {0xFF, 0xEE, 0xDD};              // 3 bytes

    ESP_LOGI(TAG, "--- Testing push_back ---");
    fv.push_back(packet1);
    fv.push_back(packet2);
    fv.push_back(packet3);

    // 4. Test GetCurrentSize
    size_t count = fv.GetCurrentSize();
    ESP_LOGI(TAG, "Current stored element count: %u (Expected: 3)", (unsigned int)count);

    // 5. Test Fetch (Positive and negative indices)
    ESP_LOGI(TAG, "--- Testing Fetch ---");
    auto fetched0 = fv.Fetch(0);
    auto fetchedLast = fv.Fetch(-1); // -1 targets the last element

    ESP_LOGI(TAG, "Fetched index 0 size: %u bytes (Expected: 5)", (unsigned int)fetched0.size());
    ESP_LOGI(TAG, "Fetched index -1 (last) size: %u bytes (Expected: 3)", (unsigned int)fetchedLast.size());

    // 6. Test modify 
    ESP_LOGI(TAG, "--- Testing modify ---");
    std::vector<uint8_t> new_data = {0x99, 0x98, 0x97, 0x96, 0x95}; // 5 bytes to match packet1
    bool mod_success = fv.modify(new_data, 0);
    ESP_LOGI(TAG, "Modify index 0 result: %s", mod_success ? "SUCCESS" : "FAILED");

    // Verify modification took effect
    auto fetchedModified = fv.Fetch(0);
    if (!fetchedModified.empty()) {
        ESP_LOGI(TAG, "Modified item first byte: 0x%02X (Expected: 0x99)", fetchedModified[0]);
    }

    // 7. Test Pop (Back: i = -1)
    ESP_LOGI(TAG, "--- Testing Pop (Back) ---");
    auto popped_back = fv.Pop(-1);
    ESP_LOGI(TAG, "Popped back size: %u bytes | Remaining elements: %u", 
             (unsigned int)popped_back.size(), (unsigned int)fv.GetCurrentSize());

    // 8. Test Pop (Front: i = 0)
    ESP_LOGI(TAG, "--- Testing Pop (Front) ---");
    auto popped_front = fv.Pop(0);
    ESP_LOGI(TAG, "Popped front size: %u bytes | Remaining elements: %u", 
             (unsigned int)popped_front.size(), (unsigned int)fv.GetCurrentSize());

    ESP_LOGI(TAG, "fileVector API test task completed successfully!");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started. Spawning fileVector test task...");
    xTaskCreate(file_vector_test_task, "file_vector_test_task", 8192, NULL, 5, NULL);
}

#endif








#if defined (FileVectorTest)

#include <stdio.h>
#include <vector>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "file-vector.hpp"
#include "file_manager.hpp"

static const char* TAG = "FILE_VECTOR_TEST";

static void file_vector_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting comprehensive fileVector API test task...");

    FileManager::init();

    std::string base_name = "opus_test_storage";
    std::string bin_file = base_name;
    std::string json_file = base_name + ".json";

    // Clean start
    if (FileManager::exists(bin_file))  FileManager::deleteFile(bin_file);
    if (FileManager::exists(json_file)) FileManager::deleteFile(json_file);

    file::fileVector fv(base_name);

    // 1. Push 5 distinct packets of varying sizes to create a rich dataset
    std::vector<uint8_t> p0 = {0xAA, 0xAA};                     // 2 bytes (Index 0)
    std::vector<uint8_t> p1 = {0x11, 0x11, 0x11};               // 3 bytes (Index 1)
    std::vector<uint8_t> p2 = {0x22, 0x22, 0x22, 0x22};         // 4 bytes (Index 2 - General middle target)
    std::vector<uint8_t> p3 = {0x33, 0x33, 0x33, 0x33, 0x33};   // 5 bytes (Index 3)
    std::vector<uint8_t> p4 = {0xFF, 0xFF};                     // 2 bytes (Index 4 - Back)

    fv.push_back(p0);
    fv.push_back(p1);
    fv.push_back(p2);
    fv.push_back(p3);
    fv.push_back(p4);

    ESP_LOGI(TAG, "Initial count: %u (Expected: 5)", (unsigned int)fv.GetCurrentSize());

    // 2. Test Fetch on a general middle index (Index 2)
    ESP_LOGI(TAG, "--- Testing Fetch on middle index (2) ---");
    auto fetched_mid = fv.Fetch(2);
    ESP_LOGI(TAG, "Fetched index 2 size: %u bytes (Expected: 4)", (unsigned int)fetched_mid.size());
    if (!fetched_mid.empty()) {
        ESP_LOGI(TAG, "Index 2 first byte: 0x%02X (Expected: 0x22)", fetched_mid[0]);
    }

    // 3. Test modify on a general middle index (Index 2)
    // Note: must match the exact size of index 2 (4 bytes)
    ESP_LOGI(TAG, "--- Testing modify on middle index (2) ---");
    std::vector<uint8_t> new_mid_data = {0x99, 0x88, 0x77, 0x66};
    bool mod_mid = fv.modify(new_mid_data, 2);
    ESP_LOGI(TAG, "Modify index 2 result: %s", mod_mid ? "SUCCESS" : "FAILED");

    // Verify modification
    auto fetched_mod_mid = fv.Fetch(2);
    if (!fetched_mod_mid.empty()) {
        ESP_LOGI(TAG, "Modified index 2 first byte: 0x%02X (Expected: 0x99)", fetched_mod_mid[0]);
    }

    // 4. Test Pop on a general middle index (Index 1)
    // This will test whether byte shifting and offset compaction work properly in the middle!
    ESP_LOGI(TAG, "--- Testing Pop on middle index (1) ---");
    size_t count_before_pop = fv.GetCurrentSize();
    auto popped_mid = fv.Pop(1);
    ESP_LOGI(TAG, "Popped middle index 1 size: %u bytes (Expected: 3)", (unsigned int)popped_mid.size());
    ESP_LOGI(TAG, "Remaining count after middle pop: %u (Expected: %u)", 
             (unsigned int)fv.GetCurrentSize(), (unsigned int)(count_before_pop - 1));

    // 5. Verify that indices shifted correctly after popping index 1 
    // (What used to be index 2 should now be at index 1!)
    auto shifted_check = fv.Fetch(1);
    if (!shifted_check.empty()) {
        ESP_LOGI(TAG, "New item at index 1 first byte: 0x%02X (Expected: 0x99 from our modified data)", shifted_check[0]);
    }

    ESP_LOGI(TAG, "Comprehensive fileVector test completed successfully!");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    xTaskCreate(file_vector_test_task, "file_vector_test_task", 8192, NULL, 5, NULL);
}

#endif





#if defined (FileManagerTest)

#include <stdio.h>
#include <vector>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Include your file manager header
#include "file_manager.hpp"

static const char* TAG = "FILE_MANAGER_TEST";

static void file_manager_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting FileManager API test task...");

    // 1. Initialize storage
    esp_err_t init_err = FileManager::init();
    if (init_err != ESP_OK) {
        ESP_LOGE(TAG, "FileManager initialization failed: 0x%x", init_err);
        vTaskDelete(NULL);
        return;
    }

    std::string test_filename = "test_data.bin";

    // Clean up any leftover file from previous runs
    if (FileManager::exists(test_filename)) {
        FileManager::deleteFile(test_filename);
    }

    // 2. Test storage statistics
    size_t total_bytes = 0, free_bytes = 0;
    if (FileManager::getStorageStats(&total_bytes, &free_bytes)) {
        ESP_LOGI(TAG, "Storage Stats -> Total: %u bytes, Free: %u bytes", 
                 (unsigned int)total_bytes, (unsigned int)free_bytes);
    } else {
        ESP_LOGW(TAG, "Failed to retrieve storage stats.");
    }

    // 3. Test writing binary data (Offset = 0: creates/overwrites file)
    ESP_LOGI(TAG, "--- Testing writeFile (New file / Offset 0) ---");
    std::vector<uint8_t> initial_data = {0x10, 0x20, 0x30, 0x40, 0x50}; // 5 bytes
    bool write_ok = FileManager::writeFile(test_filename, initial_data, 0);
    ESP_LOGI(TAG, "Write offset 0 result: %s", write_ok ? "SUCCESS" : "FAILED");

    // 4. Test reading binary data back
    ESP_LOGI(TAG, "--- Testing readFileBytes ---");
    // Read all bytes starting from offset 0 (size = -1)
    std::vector<uint8_t> read_data = FileManager::readFileBytes(test_filename, 0, -1);
    ESP_LOGI(TAG, "Read back size: %u bytes (Expected: 5)", (unsigned int)read_data.size());
    if (read_data.size() == 5) {
        ESP_LOGI(TAG, "Data matches: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", 
                 read_data[0], read_data[1], read_data[2], read_data[3], read_data[4]);
    }

    // 5. Test append mode (Offset < 0 according to your docstring)
    ESP_LOGI(TAG, "--- Testing writeFile (Append mode / Offset < 0) ---");
    std::vector<uint8_t> append_data = {0xAA, 0xBB, 0xCC}; // 3 bytes
    bool append_ok = FileManager::writeFile(test_filename, append_data, -1); // negative offset triggers append
    ESP_LOGI(TAG, "Append result: %s", append_ok ? "SUCCESS" : "FAILED");

    // Read back entire file again to see if it appended or overwritten
    std::vector<uint8_t> full_data = FileManager::readFileBytes(test_filename, 0, -1);
    ESP_LOGI(TAG, "Post-append total size: %u bytes (Expected: 8)", (unsigned int)full_data.size());
    if (full_data.size() == 8) {
        ESP_LOGI(TAG, "Append verified successfully! Last byte: 0x%02X (Expected: 0xCC)", full_data.back());
    } else {
        ESP_LOGE(TAG, "CRITICAL: Append test failed! File size is %u instead of 8. writeFile with offset < 0 might be overwriting instead of appending!", (unsigned int)full_data.size());
    }

    // 6. Test file existence & deletion
    ESP_LOGI(TAG, "--- Testing exists and deleteFile ---");
    bool file_exists = FileManager::exists(test_filename);
    ESP_LOGI(TAG, "File exists check: %s", file_exists ? "YES" : "NO");

    bool deleted = FileManager::deleteFile(test_filename);
    ESP_LOGI(TAG, "Delete file result: %s", deleted ? "SUCCESS" : "FAILED");
    ESP_LOGI(TAG, "File exists after delete: %s", FileManager::exists(test_filename) ? "YES" : "NO");

    // 7. Deinit storage
    FileManager::deinit();
    ESP_LOGI(TAG, "FileManager test task completed successfully!");
    
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "App main started. Spawning FileManager test task...");
    
    // Spawn test task with 8KB stack
    xTaskCreate(file_manager_test_task, "file_manager_test_task", 8192, NULL, 5, NULL);
}

#endif













#if defined (ShiftBytesTest1)

#include <stdio.h>
#include <vector>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "file_manager.hpp"

static const char* TAG = "SHIFT_BYTES_TEST";

static void shift_bytes_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting FileManager::shiftBytes test task...");

    FileManager::init();

    std::string test_file = "shift_test.bin";
    if (FileManager::exists(test_file)) {
        FileManager::deleteFile(test_file);
    }

    // 1. Create a file with 10 sequential bytes: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    std::vector<uint8_t> initial_data = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    FileManager::writeFile(test_file, initial_data, 0);

    ESP_LOGI(TAG, "Initial file created (10 bytes).");

    // 2. Test Left Shift (Compaction)
    // Let's shift bytes left by 2 units, starting from offset 2 up to the end.
    // Expected behavior: Bytes at index 2 and 3 ([0x02, 0x03]) should be overwritten 
    // by the subsequent bytes shifting down, shrinking the active data window.
    // Parameters: filename, shiftDistance, left (true), startOffset, endOffset
    ESP_LOGI(TAG, "--- Testing shiftBytes (Left shift by 2 starting at offset 2) ---");
    
    // Assuming signature: shiftBytes(filename, distance, left, startOffset, endOffset)
    // Let's pass startOffset = 2, endOffset = 10 (or -1 for EOF)
    FileManager::shiftBytes(test_file, 2, true, 2, -1);

    // 3. Read back the file to inspect the resulting byte sequence
    std::vector<uint8_t> result_data = FileManager::readFileBytes(test_file, 0, -1);
    
    ESP_LOGI(TAG, "Result file size after left shift: %u bytes", (unsigned int)result_data.size());
    
    std::string hex_str = "";
    for (uint8_t b : result_data) {
        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02X ", b);
        hex_str += buf;
    }
    ESP_LOGI(TAG, "Resulting bytes: %s", hex_str.c_str());

    // Clean up
    FileManager::deleteFile(test_file);
    FileManager::deinit();

    ESP_LOGI(TAG, "shiftBytes test task completed.");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    xTaskCreate(shift_bytes_test_task, "shift_bytes_test_task", 8192, NULL, 5, NULL);
}

#endif















#if defined (ShiftBytesTest3)

#include <stdio.h>
#include <vector>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "file_manager.hpp"

static const char* TAG = "SHIFT_BYTES_TEST";

static void shift_bytes_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting updated FileManager::shiftBytes test task...");

    FileManager::init();

    std::string test_file = "shift_test.bin";
    if (FileManager::exists(test_file)) {
        FileManager::deleteFile(test_file);
    }

    // 1. Create a file with 10 sequential bytes: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    std::vector<uint8_t> initial_data = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    FileManager::writeFile(test_file, initial_data, 0);

    ESP_LOGI(TAG, "Initial file created (10 bytes): 00 01 02 03 04 05 06 07 08 09");

    // 2. Test Left Shift (Compaction)
    // Scenario: We want to remove a 2-byte hole at index 2 (bytes 0x02 and 0x03).
    // - Hole starts at offset: 2
    // - Hole size (shiftDistance): 2 bytes
    // - The data we want to move starts at offset: 2 + 2 = 4 (which is 0x04)
    // We tell shiftBytes: move data starting at offset 4 left by 2 bytes.
    ESP_LOGI(TAG, "--- Testing Left Shift (moving data from offset 4 left by 2 bytes) ---");
    
    FileManager::shiftBytes(test_file, 2, true, 4, -1);

    // 3. Read back the file to inspect the shifted bytes
    std::vector<uint8_t> result_data = FileManager::readFileBytes(test_file, 0, -1);
    
    std::string hex_str = "";
    for (uint8_t b : result_data) {
        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02X ", b);
        hex_str += buf;
    }
    ESP_LOGI(TAG, "Resulting bytes: %s", hex_str.c_str());

    // Clean up
    FileManager::deleteFile(test_file);
    FileManager::deinit();

    ESP_LOGI(TAG, "shiftBytes test task completed.");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    xTaskCreate(shift_bytes_test_task, "shift_bytes_test_task", 8192, NULL, 5, NULL);
}

#endif




#if defined (ShiftBytesTest)

#include <stdio.h>
#include <vector>
#include <string>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "file_manager.hpp"

static const char* TAG = "SHIFT_BYTES_TEST";

static void print_bytes(const std::string& prefix, const std::vector<uint8_t>& data) {
    std::string hex_str = "";
    for (uint8_t b : data) {
        char buf[8];
        snprintf(buf, sizeof(buf), "0x%02X ", b);
        hex_str += buf;
    }
    ESP_LOGI(TAG, "%s: %s (size: %u)", prefix.c_str(), hex_str.c_str(), (unsigned int)data.size());
}

static void shift_bytes_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting comprehensive FileManager::shiftBytes test (Left & Right)...");

    FileManager::init();

    std::string test_file = "shift_general_test.bin";

    // ==========================================
    // TEST 1: General Left Shift (Compaction)
    // ==========================================
    if (FileManager::exists(test_file)) FileManager::deleteFile(test_file);

    std::vector<uint8_t> init_left = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    FileManager::writeFile(test_file, init_left, 0);
    print_bytes("Initial Left-Shift File", FileManager::readFileBytes(test_file, 0, -1));

    // Scenario: Remove a 2-byte block at index 3 and 4 ([0x03, 0x04])
    // - Data to move starts at offset: 5 (values [0x05...])
    // - Shift distance: 2 bytes
    // - End offset: 10 (EOF)
    size_t shift_dist = 2;
    long start_off = 5;
    long end_off = 10;

    FileManager::shiftBytes(test_file, shift_dist, true, start_off, end_off);
    FileManager::truncate(test_file, init_left.size() - shift_dist);

    print_bytes("After Left Shift (Expected: 00 01 02 05 06 07 08 09)", FileManager::readFileBytes(test_file, 0, -1));

    // ==========================================
    // TEST 2: General Right Shift (Expansion / Insertion Gap)
    // ==========================================
    if (FileManager::exists(test_file)) FileManager::deleteFile(test_file);

    std::vector<uint8_t> init_right = {0x10, 0x20, 0x30, 0x40, 0x50}; // 5 bytes
    FileManager::writeFile(test_file, init_right, 0);
    print_bytes("Initial Right-Shift File", FileManager::readFileBytes(test_file, 0, -1));

    // Scenario: Create a 2-byte insertion hole starting at index 2 (values [0x30, 0x40, 0x50] move right)
    // - Region to move starts at offset: 2
    // - Shift distance: 2 bytes
    // - End offset: 5 (EOF)
    size_t r_shift_dist = 2;
    long r_start_off = 2;
    long r_end_off = 5;

    FileManager::shiftBytes(test_file, r_shift_dist, false, r_start_off, r_end_off);

    print_bytes("After Right Shift (Expected data at offset 4+: 30 40 50)", FileManager::readFileBytes(test_file, 0, -1));

    // Clean up
    FileManager::deleteFile(test_file);
    FileManager::deinit();

    ESP_LOGI(TAG, "Comprehensive shiftBytes test task completed.");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    xTaskCreate(shift_bytes_test_task, "shift_bytes_test_task", 8192, NULL, 5, NULL);
}

#endif





















#if defined (JsonFileStoreTest)

#include <stdio.h>
#include <string>
#include <vector>
#include <cstring>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "file_manager.hpp"
#include "file-vector.hpp"
#include "Database/class-json/json-file.hpp" // Adjust header name to match your file

static const char* TAG = "JSON_FILE_STORE_TEST";

// Define two distinct plain structs for mixed type testing
struct SensorReading {
    int id;
    float value;
    uint32_t timestamp;
};

struct DeviceConfig {
    uint8_t mode;
    bool enabled;
    char name[16];
};

static void json_file_store_test_task(void* pvParameters) {
    ESP_LOGI(TAG, "Starting comprehensive JsonFileStore test task...");

    FileManager::init();

    std::string store_name = "mixed_device_storage";
    std::string bin_file = store_name;
    std::string json_file = store_name + ".json";

    // Clean start
    if (FileManager::exists(bin_file))  FileManager::deleteFile(bin_file);
    if (FileManager::exists(json_file)) FileManager::deleteFile(json_file);

    {
        file::JsonFileStore store(store_name);

        // ==========================================
        // 1. TEST SAVE & GET SIZE (Mixed Types)
        // ==========================================
        ESP_LOGI(TAG, "--- Testing save and GetSize ---");

        SensorReading s1 = {1, 23.5f, 1000};
        SensorReading s2 = {2, 24.1f, 2000};
        SensorReading s3 = {3, 25.8f, 3000};

        DeviceConfig c1 = {1, true, "SensorNodeA"};
        DeviceConfig c2 = {2, false, "SensorNodeB"};

        // Interleave saves of different types
        store.save(s1); // Sensor index 0
        store.save(c1); // Config index 0
        store.save(s2); // Sensor index 1
        store.save(c2); // Config index 1
        store.save(s3); // Sensor index 2

        ESP_LOGI(TAG, "SensorReading count: %u (Expected: 3)", (unsigned int)store.GetSize<SensorReading>());
        ESP_LOGI(TAG, "DeviceConfig count: %u (Expected: 2)", (unsigned int)store.GetSize<DeviceConfig>());

        // ==========================================
        // 2. TEST FETCH & NEGATIVE INDEXING
        // ==========================================
        ESP_LOGI(TAG, "--- Testing fetch and negative indexing ---");

        SensorReading fetched_s_first = store.fetch<SensorReading>(0);
        SensorReading fetched_s_last  = store.fetch<SensorReading>(-1); // Should be s3
        DeviceConfig  fetched_c_last  = store.fetch<DeviceConfig>(-1);  // Should be c2

        ESP_LOGI(TAG, "Sensor[0] ID: %d (Expected: 1)", fetched_s_first.id);
        ESP_LOGI(TAG, "Sensor[-1] ID: %d (Expected: 3)", fetched_s_last.id);
        ESP_LOGI(TAG, "Config[-1] name: %s (Expected: SensorNodeB)", fetched_c_last.name);

        // ==========================================
        // 3. TEST MODIFY (In-place update)
        // ==========================================
        ESP_LOGI(TAG, "--- Testing modify ---");

        SensorReading updated_s2 = {2, 99.9f, 2222};
        store.modify(updated_s2, 1); // Modify sensor at type-index 1 (which was s2)

        SensorReading verify_mod = store.fetch<SensorReading>(1);
        ESP_LOGI(TAG, "Modified Sensor[1] value: %.1f (Expected: 99.9)", verify_mod.value);

        // ==========================================
        // 4. TEST POP (Middle and End removal with shifting)
        // ==========================================
        ESP_LOGI(TAG, "--- Testing Pop ---");

        // Pop the middle sensor reading (index 1, which has value 99.9)
        SensorReading popped = store.Pop<SensorReading>(1);
        ESP_LOGI(TAG, "Popped Sensor ID: %d (Expected: 2)", popped.id);
        ESP_LOGI(TAG, "New SensorReading count: %u (Expected: 2)", (unsigned int)store.GetSize<SensorReading>());

        // Verify remaining sensor indices shifted correctly: 
        // Sensor[0] should be id 1, Sensor[1] (formerly s3) should now be id 3
        SensorReading new_s_mid = store.fetch<SensorReading>(1);
        ESP_LOGI(TAG, "New Sensor[1] ID after pop: %d (Expected: 3)", new_s_mid.id);

        // Verify DeviceConfig records were unaffected by sensor popping
        ESP_LOGI(TAG, "DeviceConfig count after sensor pop: %u (Expected: 2)", (unsigned int)store.GetSize<DeviceConfig>());
        DeviceConfig verify_config = store.fetch<DeviceConfig>(0);
        ESP_LOGI(TAG, "DeviceConfig[0] name: %s (Expected: SensorNodeA)", verify_config.name);
    }

    // Clean up
    if (FileManager::exists(bin_file))  FileManager::deleteFile(bin_file);
    if (FileManager::exists(json_file)) FileManager::deleteFile(json_file);
    FileManager::deinit();

    ESP_LOGI(TAG, "JsonFileStore test task completed successfully!");
    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    xTaskCreate(json_file_store_test_task, "json_file_store_test_task", 8192, NULL, 5, NULL);
}

#endif







#if defined (TestNewOpusClassSuccess)


#include <cstdint>
#include <cmath>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "Opus_class.hpp"

static const char* TAG = "OPUS_TEST";

static constexpr int SAMPLE_RATE = 16000;
static constexpr int CHANNELS = 2;
static constexpr int FRAME_MS = 20;
static constexpr int FRAME_SIZE =
    SAMPLE_RATE * FRAME_MS / 1000;
static constexpr int SAMPLE_COUNT =
    FRAME_SIZE * CHANNELS;

static void opus_test_task(void* arg)
{
    // Configure these arguments to match your Opus::init() signature.
    Opus opus;
    opus.init(SAMPLE_RATE, CHANNELS, 32, FRAME_MS);

    // Signed 32-bit PCM: 640 interleaved samples per frame.
    std::vector<int32_t> pcm(SAMPLE_COUNT);

    uint32_t frameNumber = 0;
    double phase = 0.0;
    constexpr double PI = 3.14159265358979323846;
    constexpr double FREQUENCY = 440.0;

    while (true) {
        // Generate a 440 Hz sine wave on both channels.
        for (int i = 0; i < FRAME_SIZE; ++i) {
            const double sample =
                0.25 * sin(phase);

            // Scale normalized audio to signed 32-bit PCM.
            const int32_t value =
                static_cast<int32_t>(
                    sample * 2147483647.0
                );

            pcm[2 * i]     = value;  // Left
            pcm[2 * i + 1] = value;  // Right

            phase += 2.0 * PI * FREQUENCY / SAMPLE_RATE;
            if (phase >= 2.0 * PI) {
                phase -= 2.0 * PI;
            }
        }

ESP_LOGW(TAG, "After encode: minimum free stack = %u bytes",
         static_cast<unsigned>(
             uxTaskGetStackHighWaterMark(nullptr)));

        // Encode using your existing Opus class.
        const std::vector<unsigned char> packet =
            opus.encode(pcm.data());


ESP_LOGW(TAG, "After encode: minimum free stack = %u bytes",
         static_cast<unsigned>(
             uxTaskGetStackHighWaterMark(nullptr)));



        if (packet.empty()) {
            ESP_LOGE(TAG, "Encoding failed at frame %lu",
                     static_cast<unsigned long>(frameNumber));
            // Continue to see whether the failure repeats.
        }

        ++frameNumber;

        if ((frameNumber % 100) == 0) {
            ESP_LOGI(TAG, "Encoded %lu frames",
                     static_cast<unsigned long>(frameNumber));
        }

        // Simulate a 20 ms audio frame interval.
        vTaskDelay(pdMS_TO_TICKS(FRAME_MS));
    }
}

 

extern "C" void app_main(void) {
BaseType_t result = xTaskCreate(
    opus_test_task,
    "opus_test",
    49152,
    nullptr,
    5,
    nullptr
);

ESP_LOGI(TAG, "Task creation result: %ld",
         static_cast<long>(result));
}
#endif














#if defined(TestSingleFile) // Entire test suite compiles only when this macro is defined



#include "Database/fileManager1/file_manager.hpp" // Level 1 filesystem APIs
#include "single_file.hpp" // Level 2 SingleFile class
#include "esp_log.h"        // ESP-IDF logging library
#include <string>           // std::string
#include <vector>           // std::vector
#include <algorithm>        // std::min, std::max

// Define the macro to enable test execution
#define TestSingleFile 1



namespace { // Anonymous namespace to keep test utilities local to this file

static const char* TAG = "TestSingleFile"; // Log filter tag
static const char* TEST_FILE = "test_run.bin"; // Dedicated test file name

// Global test counters
static int s_tests_run = 0;    // Tracks total assertions executed
static int s_tests_passed = 0; // Tracks successful assertions
static int s_tests_failed = 0; // Tracks failed assertions

// Assertion macro reporting PASS/FAIL along with the line number and expression
#define TEST_CHECK(condition, desc) do { \
    s_tests_run++; \
    if (condition) { \
        s_tests_passed++; \
        ESP_LOGI(TAG, "  [PASS] %s", desc); \
    } else { \
        s_tests_failed++; \
        ESP_LOGE(TAG, "  [FAIL] %s (Line %d: %s)", desc, __LINE__, #condition); \
    } \
} while (0)

// Helper to remove any leftover test file from prior runs
void cleanupTestFile() {
    if (FileManager::exists(TEST_FILE)) {
        FileManager::deleteFile(TEST_FILE);
    }
}

// ----------------------------------------------------------------------------
// Test 1: Basic Write, Read, and File Size
// ----------------------------------------------------------------------------
void test_basic_write_read() {
    ESP_LOGI(TAG, "--- Test: Basic Write, Read, and File Size ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);
        std::string payload = "Hello, ESP32 SPIFFS!";

        // 1. Write payload starting at offset 0
        TEST_CHECK(file.writeFile(payload, 0), "Write initial payload at offset 0");

        // 2. Validate fileSize matches written byte count
        TEST_CHECK(file.fileSize() == payload.size(), "fileSize() matches written string length");

        // 3. Read back exact bytes
        std::vector<uint8_t> readBytes = file.readFileBytes(0, payload.size());
        std::string result(readBytes.begin(), readBytes.end());
        TEST_CHECK(result == payload, "readFileBytes() matches exact data written");
    }

    cleanupTestFile();
}

// ----------------------------------------------------------------------------
// Test 2: Multiple Consecutive Writes & Append Mode (Handle Reuse)
// ----------------------------------------------------------------------------
void test_consecutive_appends() {
    ESP_LOGI(TAG, "--- Test: Consecutive Appends (Handle Reuse) ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);

        // Multiple consecutive writes using the already open file handle
        TEST_CHECK(file.writeFile("Line1\n", -1), "Append Line 1 (offset = -1)");
        TEST_CHECK(file.writeFile("Line2\n", -1), "Append Line 2 (offset = -1)");
        TEST_CHECK(file.writeFile("Line3\n", -1), "Append Line 3 (offset = -1)");

        // 6 bytes * 3 = 18 bytes
        TEST_CHECK(file.fileSize() == 18, "Combined file size is 18 bytes");

        // Read entire content to EOF (size = -1)
        std::vector<uint8_t> all = file.readFileBytes(0, -1);
        std::string fullText(all.begin(), all.end());
        TEST_CHECK(fullText == "Line1\nLine2\nLine3\n", "Read entire file to EOF matches appends");
    }

    cleanupTestFile();
}

// ----------------------------------------------------------------------------
// Test 3: Zero-Padding when Seeking Past EOF
// ----------------------------------------------------------------------------
void test_zero_padding() {
    ESP_LOGI(TAG, "--- Test: Zero-Padding when Seeking Past EOF ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);
        file.writeFile("HEAD", 0); // 4 bytes

        // Write "TAIL" at offset 8 (bytes 4..7 must be auto-filled with 0x00)
        TEST_CHECK(file.writeFile("TAIL", 8), "Write 'TAIL' at offset 8 (gap = 4 bytes)");
        TEST_CHECK(file.fileSize() == 12, "File size after gap write is 12 bytes");

        // Verify gap bytes (offset 4, size 4) are indeed 0x00
        std::vector<uint8_t> gap = file.readFileBytes(4, 4);
        bool allZero = (gap.size() == 4);
        for (uint8_t b : gap) {
            if (b != 0) { allZero = false; break; }
        }
        TEST_CHECK(allZero, "Intermediate gap bytes are verified 0x00 padded");

        // Verify data after gap is intact
        std::vector<uint8_t> tail = file.readFileBytes(8, 4);
        std::string tailStr(tail.begin(), tail.end());
        TEST_CHECK(tailStr == "TAIL", "Data written after gap is intact");
    }

    cleanupTestFile();
}

// ----------------------------------------------------------------------------
// Test 4: Read Boundaries and Clamping
// ----------------------------------------------------------------------------
void test_read_edge_cases() {
    ESP_LOGI(TAG, "--- Test: Read Boundaries and Clamping ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);
        file.writeFile("0123456789", 0); // 10 bytes

        // size = -1 should read all remaining bytes to EOF
        std::vector<uint8_t> toEnd = file.readFileBytes(5, -1);
        std::string strToEnd(toEnd.begin(), toEnd.end());
        TEST_CHECK(strToEnd == "56789", "size = -1 reads cleanly to EOF");

        // Requesting more bytes than exist should clamp to EOF
        std::vector<uint8_t> clamped = file.readFileBytes(7, 100);
        std::string strClamped(clamped.begin(), clamped.end());
        TEST_CHECK(strClamped == "789", "Oversized read length is clamped to EOF");

        // Reading at offset == fileSize should return an empty buffer
        std::vector<uint8_t> oob = file.readFileBytes(10, 5);
        TEST_CHECK(oob.empty(), "Offset == fileSize returns empty buffer");

        // Reading past fileSize should return an empty buffer
        std::vector<uint8_t> wayBeyond = file.readFileBytes(50, 5);
        TEST_CHECK(wayBeyond.empty(), "Offset > fileSize returns empty buffer");
    }

    cleanupTestFile();
}

// ----------------------------------------------------------------------------
// Test 5: In-Place Shift Left (Compaction)
// ----------------------------------------------------------------------------
void test_shift_left() {
    ESP_LOGI(TAG, "--- Test: In-Place Shift Left (Compaction) ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);
        // Initial layout (16 bytes): "AAAABBBBCCCCDDDD"
        file.writeFile("AAAABBBBCCCCDDDD", 0);

        // Shift "CCCCDDDD" (offset 8 to 16) left by 4 -> overwrites "BBBB" (4..11)
        file.shiftBytes(4, true, 8, 16);

        // Read back bytes 4..11
        std::vector<uint8_t> shifted = file.readFileBytes(4, 8);
        std::string result(shifted.begin(), shifted.end());
        TEST_CHECK(result == "CCCCDDDD", "Left shift correctly moves region forward");
    }

    cleanupTestFile();
}

// ----------------------------------------------------------------------------
// Test 6: In-Place Shift Right (Expansion)
// ----------------------------------------------------------------------------
void test_shift_right() {
    ESP_LOGI(TAG, "--- Test: In-Place Shift Right (Expansion) ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);
        // Initial layout (10 bytes): "0123456789"
        file.writeFile("0123456789", 0);

        // Shift "0123" (0 to 4) right by 2 -> destination starts at index 2
        file.shiftBytes(2, false, 0, 4);

        // Bytes 2..5 should now contain "0123"
        std::vector<uint8_t> shifted = file.readFileBytes(2, 4);
        std::string result(shifted.begin(), shifted.end());
        TEST_CHECK(result == "0123", "Right shift correctly moves region backward");
    }

    cleanupTestFile();
}

// ----------------------------------------------------------------------------
// Test 7: Truncate and Delete File
// ----------------------------------------------------------------------------
void test_truncate_and_delete() {
    ESP_LOGI(TAG, "--- Test: Truncate and Delete File ---");
    cleanupTestFile();

    {
        FileManager::SingleFile file(TEST_FILE);
        file.writeFile("0123456789ABCDEF", 0); // 16 bytes
        TEST_CHECK(file.fileSize() == 16, "File size is 16 bytes before truncate");

        // Truncate to 5 bytes
        TEST_CHECK(file.truncate(5), "ftruncate down to 5 bytes returned success");
        TEST_CHECK(file.fileSize() == 5, "fileSize() is updated to 5 bytes");

        std::vector<uint8_t> rem = file.readFileBytes(0, -1);
        std::string remStr(rem.begin(), rem.end());
        TEST_CHECK(remStr == "01234", "Content matches truncated boundary");

        // Self deletion
        TEST_CHECK(file.deleteFile(), "deleteFile() on self returned success");
        TEST_CHECK(!FileManager::exists(TEST_FILE), "File confirmed deleted via FileManager::exists()");
    }
}

// ----------------------------------------------------------------------------
// Test Runner
// ----------------------------------------------------------------------------
void runSingleFileTests() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "       RUNNING COMPREHENSIVE SINGLEFILE TESTS     ");
    ESP_LOGI(TAG, "==================================================");

    s_tests_run = 0;
    s_tests_passed = 0;
    s_tests_failed = 0;

    test_basic_write_read();
    test_consecutive_appends();
    test_zero_padding();
    test_read_edge_cases();
    test_shift_left();
    test_shift_right();
    test_truncate_and_delete();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "RESULTS: %d / %d assertions passed (%d failed)", 
             s_tests_passed, s_tests_run, s_tests_failed);
    ESP_LOGI(TAG, "==================================================");
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// Main Application Entry Point
// ----------------------------------------------------------------------------
extern "C" void app_main(void) {
    // 1. Mount filesystem partition (Level 1)
    esp_err_t err = FileManager::init("/spiffs", "my_files", 5);
    if (err != ESP_OK) {
        ESP_LOGE("MAIN", "Filesystem initialization failed! Check partition table.");
        return;
    }

    // 2. Run the test suite
    runSingleFileTests();

    // 3. Unmount SPIFFS
    FileManager::deinit();
}

#endif // TestSingleFile





















































#if defined(TestFIFO) // Encloses entire test suite under the macro



#include "file_manager.hpp" // Level 1 filesystem mount and deletion utilities
#include "single_file.hpp"  // Level 2 SingleFile utility for size verification
#include "FIFO_file.hpp"    // FIFO class under test
#include "esp_log.h"        // ESP-IDF console logging
#include <vector>           // std::vector
#include <string>           // std::string
#include <cstring>          // std::memcmp

// ============================================================================
// CONFIGURATION MACRO (Comment out to disable test compilation)
// ============================================================================
#define TestFIFO 1 // Activates the FIFO test suite



namespace { // Anonymous namespace to keep test utilities private

static const char* TAG = "TestFIFO"; // Log identifier for test output
static const char* TEST_FILE = "fifo_test.bin"; // Dedicated test file on SPIFFS

// Global assertion counters
static int s_tests_run = 0;    // Number of assertions executed
static int s_tests_passed = 0; // Number of assertions passed
static int s_tests_failed = 0; // Number of assertions failed

// Assertion helper macro that records file, line, and status
#define TEST_CHECK(condition, desc) do { \
    s_tests_run++; \
    if (condition) { \
        s_tests_passed++; \
        ESP_LOGI(TAG, "  [PASS] %s", desc); \
    } else { \
        s_tests_failed++; \
        ESP_LOGE(TAG, "  [FAIL] %s (Line %d: %s)", desc, __LINE__, #condition); \
    } \
} while (0)

// Helper to remove any leftover test file from prior runs
void cleanupTestFile() {
    if (FileManager::exists(TEST_FILE)) { // Checks if test file exists
        FileManager::deleteFile(TEST_FILE); // Deletes file to ensure clean test state
    } // Closes check
} // Closes cleanupTestFile

// ----------------------------------------------------------------------------
// Test 1: Basic Push, Pop, and Empty Queue
// ----------------------------------------------------------------------------
void test_basic_push_pop() {
    ESP_LOGI(TAG, "--- Test: Basic Push, Pop, and Empty Queue ---"); // Log banner
    cleanupTestFile(); // Ensure clean slate

    { // Scope block
        file::FIFO queue(TEST_FILE); // Create FIFO instance

        TEST_CHECK(queue.empty(), "Newly created FIFO reports empty"); // Assert initially empty

        std::vector<uint8_t> p1 = {0x01, 0x02, 0x03}; // Test packet 1
        std::vector<uint8_t> p2 = {0xAA, 0xBB, 0xCC, 0xDD}; // Test packet 2

        queue.push_back(p1); // Push packet 1
        queue.push_back(p2); // Push packet 2
        queue.flush(); // Flush writes to disk

        TEST_CHECK(!queue.empty(), "FIFO is not empty after pushing packets"); // Assert not empty

        queue.begin_pop_mode(); // Switch to pop mode

        std::vector<uint8_t> out1 = queue.Pop(); // Pop packet 1
        TEST_CHECK(out1 == p1, "First popped packet matches packet 1"); // Assert exact payload match

        std::vector<uint8_t> out2 = queue.Pop(); // Pop packet 2
        TEST_CHECK(out2 == p2, "Second popped packet matches packet 2"); // Assert exact payload match

        TEST_CHECK(queue.empty(), "FIFO reports empty after all packets are popped"); // Assert empty

        std::vector<uint8_t> out3 = queue.Pop(); // Attempt popping empty queue
        TEST_CHECK(out3.empty(), "Popping an empty FIFO returns empty vector"); // Assert safe empty return
    } // Closes FIFO instance

    cleanupTestFile(); // Clean up test file
} // Closes test_basic_push_pop

// ----------------------------------------------------------------------------
// Test 2: Cache Overflow / Batching Across Multiple Blocks
// ----------------------------------------------------------------------------
void test_cache_overflow() {
    ESP_LOGI(TAG, "--- Test: Cache Overflow (Over 1024 Bytes) ---"); // Log banner
    cleanupTestFile(); // Ensure clean slate

    { // Scope block
        file::FIFO queue(TEST_FILE); // Create FIFO instance

        constexpr size_t NUM_PACKETS = 15; // 15 packets
        constexpr size_t PACKET_SIZE = 100; // 100 bytes each -> total 1500 bytes (exceeds 1024-byte cache)

        std::vector<std::vector<uint8_t>> expectedPackets; // Track expected payloads

        for (size_t i = 0; i < NUM_PACKETS; i++) { // Loop to push packets
            std::vector<uint8_t> packet(PACKET_SIZE, static_cast<uint8_t>(i + 1)); // Payload filled with (i+1)
            expectedPackets.push_back(packet); // Store expected payload
            queue.push_back(packet); // Enqueue packet into FIFO (triggers auto-flush when cache fills)
        } // Closes loop

        queue.flush(); // Flush any remaining cached data

        queue.begin_pop_mode(); // Switch to pop mode

        bool allMatched = true; // Tracking flag
        for (size_t i = 0; i < NUM_PACKETS; i++) { // Loop to pop packets
            std::vector<uint8_t> out = queue.Pop(); // Dequeue packet
            if (out != expectedPackets[i]) { // Verify exact payload match
                allMatched = false; // Flag mismatch
                break; // Abort
            } // Closes mismatch check
        } // Closes loop

        TEST_CHECK(allMatched, "All 15 packets popped in exact order across cache flushes"); // Assert match
        TEST_CHECK(queue.empty(), "Queue is empty after reading all overflowed packets"); // Assert empty
    } // Closes FIFO instance

    cleanupTestFile(); // Clean up test file
} // Closes test_cache_overflow

// ----------------------------------------------------------------------------
// Test 3: Oversized Packet & Order Integrity
// ----------------------------------------------------------------------------
void test_oversized_packet_order() {
    ESP_LOGI(TAG, "--- Test: Oversized Packet & Order Integrity ---"); // Log banner
    cleanupTestFile(); // Ensure clean slate

    { // Scope block
        file::FIFO queue(TEST_FILE); // Create FIFO instance

        std::vector<uint8_t> p1 = {0x11, 0x22}; // Small packet (sits in cache)
        std::vector<uint8_t> p2(1200, 0xEE);    // Oversized packet (1200 bytes > 1024-byte cache capacity)
        std::vector<uint8_t> p3 = {0x33, 0x44}; // Small packet (pushed after oversized packet)

        queue.push_back(p1); // Enqueue small packet 1 into cache_
        queue.push_back(p2); // Enqueue oversized packet 2 (MUST flush p1 first, then write p2!)
        queue.push_back(p3); // Enqueue small packet 3 into fresh cache_
        queue.flush(); // Commit all to storage

        queue.begin_pop_mode(); // Switch to pop mode

        std::vector<uint8_t> out1 = queue.Pop(); // Pop 1st packet
        TEST_CHECK(out1 == p1, "Packet 1 popped first (cached data was not bypassed!)"); // Assert p1 first

        std::vector<uint8_t> out2 = queue.Pop(); // Pop 2nd packet
        TEST_CHECK(out2 == p2, "Packet 2 popped second (oversized packet intact)"); // Assert p2 second

        std::vector<uint8_t> out3 = queue.Pop(); // Pop 3rd packet
        TEST_CHECK(out3 == p3, "Packet 3 popped third (post-oversized packet intact)"); // Assert p3 third

        TEST_CHECK(queue.empty(), "FIFO is empty after popping all three packets"); // Assert empty
    } // Closes FIFO instance

    cleanupTestFile(); // Clean up test file
} // Closes test_oversized_packet_order

// ----------------------------------------------------------------------------
// Test 4: Head Persistence Across Reboots / Re-instantiation
// ----------------------------------------------------------------------------
void test_head_persistence_across_reboots() {
    ESP_LOGI(TAG, "--- Test: Head Persistence Across Reboots ---"); // Log banner
    cleanupTestFile(); // Ensure clean slate

    std::vector<uint8_t> p1 = {0xAA, 0x01}; // Packet 1
    std::vector<uint8_t> p2 = {0xBB, 0x02}; // Packet 2
    std::vector<uint8_t> p3 = {0xCC, 0x03}; // Packet 3

    // Phase 1: Push 3 packets, pop 1, and destroy the object (simulating reboot)
    { // Scope block 1
        file::FIFO queue(TEST_FILE); // Instantiate first FIFO object
        queue.push_back(p1); // Push packet 1
        queue.push_back(p2); // Push packet 2
        queue.push_back(p3); // Push packet 3
        queue.flush(); // Commit writes

        queue.begin_pop_mode(); // Switch to pop mode
        std::vector<uint8_t> out1 = queue.Pop(); // Pop packet 1 (head_ advances past p1)
        TEST_CHECK(out1 == p1, "Phase 1: Successfully popped packet 1"); // Assert p1
        queue.flush(); // Persist advanced head_ to disk header
    } // FIFO instance destroyed here; destructor commits final state

    // Phase 2: Create a brand new FIFO instance on the same file (simulating post-reboot)
    { // Scope block 2
        file::FIFO queue(TEST_FILE); // Boot up: constructor must load persistent head_ from disk
        TEST_CHECK(!queue.empty(), "Phase 2: Reloaded FIFO recognizes pending unread packets"); // Assert not empty

        queue.begin_pop_mode(); // Switch to pop mode
        std::vector<uint8_t> out2 = queue.Pop(); // Pop next packet
        TEST_CHECK(out2 == p2, "Phase 2: Successfully resumed and popped packet 2"); // Assert p2 popped

        std::vector<uint8_t> out3 = queue.Pop(); // Pop next packet
        TEST_CHECK(out3 == p3, "Phase 2: Successfully popped packet 3"); // Assert p3 popped

        TEST_CHECK(queue.empty(), "Phase 2: Queue is now completely empty"); // Assert empty
    } // Closes scope block 2

    cleanupTestFile(); // Clean up test file
} // Closes test_head_persistence_across_reboots

// ----------------------------------------------------------------------------
// Test 5: Instant Truncation on Exhaustion (Zero Shift Wear)
// ----------------------------------------------------------------------------
void test_instant_truncation_on_exhaustion() {
    ESP_LOGI(TAG, "--- Test: Instant Truncation on Exhaustion ---"); // Log banner
    cleanupTestFile(); // Ensure clean slate

    { // Scope block
        file::FIFO queue(TEST_FILE); // Create FIFO instance

        // Push 10 packets of 50 bytes each
        for (int i = 0; i < 10; i++) { // Loop
            queue.push_back(std::vector<uint8_t>(50, 0x77)); // Push packet
        } // Closes loop
        queue.flush(); // Commit to disk

        // Verify file expanded beyond header
        FileManager::SingleFile checker(TEST_FILE); // Open inspection handle
        TEST_CHECK(checker.fileSize() > 8, "File size grew with pushed packets"); // Assert grew

        // Drain the queue completely
        queue.begin_pop_mode(); // Switch to pop mode
        while (!queue.empty()) { // Drain loop
            queue.Pop(); // Pop packet
        } // Closes loop

        // When exhausted, file should instantly truncate back to HEADER_SIZE (8 bytes)
        TEST_CHECK(checker.fileSize() == 8, "Exhausted FIFO instantly truncated to 8 bytes (Header size)"); // Assert 8 bytes
    } // Closes FIFO instance

    cleanupTestFile(); // Clean up test file
} // Closes test_instant_truncation_on_exhaustion

// ----------------------------------------------------------------------------
// Test 6: Compaction on Large Dead Gap (>4096 Bytes)
// ----------------------------------------------------------------------------
void test_compaction_on_large_gap() {
    ESP_LOGI(TAG, "--- Test: Compaction on Large Dead Gap ---"); // Log banner
    cleanupTestFile(); // Ensure clean slate

    { // Scope block
        file::FIFO queue(TEST_FILE); // Create FIFO instance

        // Push 50 packets of 100 bytes each (50 * 104 = 5200 bytes on disk)
        for (int i = 0; i < 50; i++) { // Loop
            queue.push_back(std::vector<uint8_t>(100, static_cast<uint8_t>(i))); // Push packet
        } // Closes loop
        queue.flush(); // Commit to disk

        queue.begin_pop_mode(); // Switch to pop mode

        // Pop 45 packets: (45 * 104 = 4680 bytes of dead gap > GAP_THRESHOLD of 4096)
        for (int i = 0; i < 45; i++) { // Loop
            queue.Pop(); // Pop packet
        } // Closes loop

        // Switch to push mode: this triggers maybeCompact(), which should detect gap >= 4096
        queue.begin_push_mode(); // Switches mode and executes compactDisk()

        // 5 unread packets (indices 45..49) must survive the shift completely intact
        queue.begin_pop_mode(); // Switch back to pop mode

        bool remainingValid = true; // Validation flag
        for (int i = 45; i < 50; i++) { // Loop over remaining 5 packets
            std::vector<uint8_t> out = queue.Pop(); // Pop packet
            std::vector<uint8_t> expected(100, static_cast<uint8_t>(i)); // Expected payload
            if (out != expected) { // Compare
                remainingValid = false; // Flag error
                break; // Abort
            } // Closes check
        } // Closes loop

        TEST_CHECK(remainingValid, "Remaining 5 packets are 100% intact after 4KB dead space shift"); // Assert intact
        TEST_CHECK(queue.empty(), "Queue is empty after popping surviving packets"); // Assert empty
    } // Closes FIFO instance

    cleanupTestFile(); // Clean up test file
} // Closes test_compaction_on_large_gap

} // anonymous namespace

// ----------------------------------------------------------------------------
// Global FIFO Test Runner
// ----------------------------------------------------------------------------
void runFIFOTests() { // Test runner function
    ESP_LOGI(TAG, "=================================================="); // Header banner
    ESP_LOGI(TAG, "           RUNNING COMPREHENSIVE FIFO TESTS       "); // Header title
    ESP_LOGI(TAG, "=================================================="); // Header separator

    s_tests_run = 0; // Reset total counter
    s_tests_passed = 0; // Reset pass counter
    s_tests_failed = 0; // Reset fail counter

    // Run test cases sequentially
    test_basic_push_pop(); // Run Test 1
    test_cache_overflow(); // Run Test 2
    test_oversized_packet_order(); // Run Test 3
    test_head_persistence_across_reboots(); // Run Test 4
    test_instant_truncation_on_exhaustion(); // Run Test 5
    test_compaction_on_large_gap(); // Run Test 6

    ESP_LOGI(TAG, "=================================================="); // Footer banner
    ESP_LOGI(TAG, "FIFO RESULTS: %d / %d assertions passed (%d failed)", // Summary output
             s_tests_passed, s_tests_run, s_tests_failed); // Variables
    ESP_LOGI(TAG, "=================================================="); // Footer separator
} // Closes runFIFOTests

// ============================================================================
// MAIN APPLICATION ENTRY POINT
// ============================================================================
extern "C" void app_main(void) { // ESP-IDF application entry point
    // 1. Mount filesystem partition (Level 1)
    esp_err_t err = FileManager::init("/spiffs", "my_files", 5); // Mount SPIFFS
    if (err != ESP_OK) { // Validate mount
        ESP_LOGE("MAIN", "Filesystem init failed! Check partition table."); // Log error
        return; // Abort
    } // Closes mount check

    // 2. Execute the test suite
    runFIFOTests(); // Run all FIFO test cases

    // 3. Unmount SPIFFS
    FileManager::deinit(); // Unmount filesystem cleanly
} // Closes app_main

#endif // TestFIFO