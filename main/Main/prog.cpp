  
#include<memory>
#include "Network/Net manager.hpp"


#include <iostream>




constexpr uint32_t conv_id = 2222222;
const char* host ="emily";
 const char* port="8080";
 const char* ssid="Dalintern device";
  const char* password= "cykcykcyk";



extern "C" {

  void app_run_loop(){
std::shared_ptr<int > sd = std::make_shared<int>(333);
std::shared_ptr<Network::manager> s_NetworkManager = std::make_shared<Network::manager>(conv_id);
 
std::cout<<"Heloo"<<std::endl;
 bool success = s_NetworkManager->initWifi(ssid,password);

bool success2=  s_NetworkManager->try_connectWithServer(   host,   port);




}
}


/*
E (901) sdmmc_common: sdmmc_init_ocr: send_op_cond (1) returned 0x107
E (901) vfs_fat_sdmmc: sdmmc_card_init failed (0x107).
E (901) vfs_fat_sdmmc: esp_vfs_fat_sdmmc_sdcard_init failed (0x107).
E (901) board: Failed to initialize the card (ESP_ERR_TIMEOUT). Make sure SD card lines have pull-up resistors in place.
I (901) ++++++++++++: success esp_sdcard_init
I (901) MODEL_LOADER: The storage free size is 24576 KB
I (901) MODEL_LOADER: The partition size is 5900 KB
I (901) MODEL_LOADER: Successfully load srmodels
I (901) AFE_CONFIG: Set WakeNet Model: wn9_hiesp
I (901) AFE_CONFIG: Set VAD Model: vadnet1_medium
MC Quantized wakenet9: wakeNet9_v1h24_Hi,ESP_3_0.63_0.635, tigger:v4, mode:4, p:0
I (1041) AFE: AFE Version: (2MIC_V251128)
I (1041) AFE: Input PCM Config: total 4 channels(2 microphone, 1 playback), sample rate:16000
I (1041) AFE: AFE Pipeline: [input] -> |AEC(SR_LOW_COST, NLP_OFF)| -> |SE(BSS)| -> |WakeNet(wn9_hiesp,)| -> [output]
multinet:mn5q8_cn
I (1071) ++++++++++++: success Speech_Init
I (1071) ++++++++++++: success Speech_register_callback
I (1071) ASP_POOL: Dest rate:16000
I (1071) ASP_POOL: Dest channels:2
I (1071) ESP_GMF_TASK: Waiting to run... [tsk:TSK_0x3fcbe7b8-0x3fcbe7b8, wk:0x0, run:0]
I (1071) ++++++++++++: success Audio_Play_Init
I (1071) rgb_led_driver: Created LED strip object with RMT backend
I (1071) ++++++++++++: success RGB_Example
I (1071) ++++++++++++: success key_module_init
I (1071) ++++++++++++: success key_register_callback
E (1071) SD: Path: </sdcard> does not exist
file_count=0
I (1071) ++++++++++++: success Search_mp3_Music

assert failed: tcpip_send_msg_wait_sem /IDF/components/lwip/lwip/src/api/tcpip.c:454 (Invalid mbox)


Backtrace: 0x4038199d:0x3fcad6e0 0x40381965:0x3fcad700 0x40380105:0x3fcad720 0x42109105:0x3fcad840 0x421159a9:0x3fcad8d0 0x4210709e:0x3fcad960 0x4200f6d2:0x3fcad9f0 0x4200f8ad:0x3fcada70 0x4200f4b1:0x3fcadac0 0x4200f596:0x3fcadb00 0x4200f5c2:0x3fcadb40 0x4200f5eb:0x3fcadb60 0x4200ec50:0x3fcadba0 0x421a2644:0x3fcadbc0 0x40384ad9:0x3fcadbf0
--- 0x4038199d: panic_abort at /home/emily/.espressif/v6.1/esp-idf/components/esp_system/panic.c:500
--- 0x40381965: esp_system_abort at /home/emily/.espressif/v6.1/esp-idf/components/esp_system/port/esp_system_chip.c:87
--- 0x40380105: __assert_func at /home/emily/.espressif/v6.1/esp-idf/components/esp_libc/src/assert.c:81
--- 0x42109105: tcpip_send_msg_wait_sem at /home/emily/.espressif/v6.1/esp-idf/components/lwip/lwip/src/api/tcpip.c:454
--- 0x421159a9: netconn_gethostbyname_addrtype_n at /home/emily/.espressif/v6.1/esp-idf/components/lwip/lwip/src/api/api_lib.c:1374
--- 0x4210709e: lwip_getaddrinfo at /home/emily/.espressif/v6.1/esp-idf/components/lwip/lwip/src/api/netdb.c:495
--- 0x4200f6d2: Network::setup_production_udp_socket(char const*, char const*) at /home/emily/.espressif/v6.1/esp-idf/components/lwip/include/lwip/netdb.h:45(inlined by) setup_production_udp_socket at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/Network/kcp-class/KcpClient.cpp:38
--- 0x4200f8ad: Network::KcpClient::initialize(char const*, char const*) at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/Network/kcp-class/KcpClient.cpp:136
--- 0x4200f4b1: Network::manager::manager(unsigned long, char const*, char const*) at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/Network/Net manager.hpp:21
--- 0x4200f596: void std::_Construct<Network::manager, unsigned long const&, char const*&, char const*&>(Network::manager*, unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/stl_construct.h:133(inlined by) void std::allocator_traits<std::allocator<void> >::construct<Network::manager, unsigned long const&, char const*&, char const*&>(std::allocator<void>&, Network::manager*, unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/alloc_traits.h:805(inlined by) std::_Sp_counted_ptr_inplace<Network::manager, std::allocator<void>, (__gnu_cxx::_Lock_policy)2>::_Sp_counted_ptr_inplace<unsigned long const&, char const*&, char const*&>(std::allocator<void>, unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/shared_ptr_base.h:606(inlined by) std::__shared_count<(__gnu_cxx::_Lock_policy)2>::__shared_count<Network::manager, std::allocator<void>, unsigned long const&, char const*&, char const*&>(Network::manager*&, std::_Sp_alloc_shared_tag<std::allocator<void> >, unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/shared_ptr_base.h:969
--- 0x4200f5c2: std::__shared_ptr<Network::manager, (__gnu_cxx::_Lock_policy)2>::__shared_ptr<std::allocator<void>, unsigned long const&, char const*&, char const*&>(std::_Sp_alloc_shared_tag<std::allocator<void> >, unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/shared_ptr_base.h:1719(inlined by) std::shared_ptr<Network::manager>::shared_ptr<std::allocator<void>, unsigned long const&, char const*&, char const*&>(std::_Sp_alloc_shared_tag<std::allocator<void> >, unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/shared_ptr.h:463
--- 0x4200f5eb: std::shared_ptr<Network::manager> std::make_shared<Network::manager, unsigned long const&, char const*&, char const*&>(unsigned long const&, char const*&, char const*&) at /home/emily/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/xtensa-esp-elf/include/c++/15.2.0/bits/shared_ptr.h:1008(inlined by) app_run_loop at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/Main/prog.cpp:23
--- 0x4200ec50: app_main at /home/emily/hardware-proj/ESP32-S3-AUDIO-Board-Demo/ESP-IDF/factory_01/main/main.c:243
--- 0x421a2644: main_task at /home/emily/.espressif/v6.1/esp-idf/components/freertos/app_startup.c:206
--- 0x40384ad9: vPortTaskWrapper at /home/emily/.espressif/v6.1/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:147




ELF file SHA256: d704f3dda

Rebooting...
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0xc (RTC_SW_CPU_RST),boot:0x28 (SPI_FAST_FLASH_BOOT)
Saved PC:0x40381b6d
--- 0x40381b6d: esp_restart_noos at /home/emily/.espressif/v6.1/esp-idf/components/esp_system/port/soc/esp32s3/system_internal.c:171
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce2820,len:0x16bc
load:0x403c8700,len:0xf44
load:0x403cb700,len:0x31f4
entry 0x403c8928
*/