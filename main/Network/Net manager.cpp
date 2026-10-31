// // // // #include "Net manager.hpp"
// // // // #include <memory>
// // // // #include <vector>
// // // // #include "encryption/interface.hpp"

// // // // namespace Network { 

// // // //     // Internal state variables (formerly class members)
  
    
// // // //     static std::unique_ptr<KcpClient> u_KcpClient;
// // // //     static std::string fileName = "packageStoredFile";
// // // //   //  static std::unique_ptr<file::fileVector> packageStoredFile = std::make_unique<file::fileVector>(fileName);
// // // //     static uint32_t conv_id; 


// // // // //TODO:this is error now,since we store un-encrypted data here;
// // // //     // static void  try_send_disk_package(){
// // // //     //     if(!wifi::isConnected())return;
// // // //     //     if(!completeHandshake)return;
// // // //     //             // has wifi; we check the unsend one;
// // // //     //         const auto currentSize = packageStoredFile->GetCurrentSize();
// // // //     //         for(int i=0;i<currentSize;++i){
// // // //     //           auto unsend_ = packageStoredFile->Pop();
 
// // // //     //                 auto Opt_encrypted_data = encryption::aes_gcm_encrypt(unsend_,aes_key);
// // // //     //     if (!Opt_encrypted_data.has_value()) return  ;

// // // //     //     auto encrypted_data = Opt_encrypted_data.value();

// // // //     //                // we encrypt here;
// // // //     //     int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
// // // //     //     if (r == -1) {
// // // //     //         ESP_LOGE(TAG, "send failed, reason: unknown");
// // // //     //     }
// // // //     //     return  ;

// // // //     //         }
 
// // // //     // }
// // // //     // Initialization (replaces the constructor)
// // // //     void init( const char* host, const char* port) {
// // // //          conv_id = 0;
 
// // // //         conv_id = esp_random();
 
// // // //         // Initialize your KcpClient here, e.g.:
// // // //          u_KcpClient = std::make_unique<KcpClient>(conv_id);
// // // //         u_KcpClient->initialize(host,port);
   
// // // //    //try_send_disk_package();

// // // //     }

// // // //     void close_connection() {
// // // //         if (u_KcpClient) {
// // // //             u_KcpClient->close_connection();
// // // //         }
// // // //     }

// // // //     // Core Tick & Data Flow Methods
// // // //     void update(uint32_t current_ms) {
// // // //         if (u_KcpClient) {
// // // //             u_KcpClient->update(current_ms);
// // // //         }
 
// // // //   // try_send_disk_package();


// // // //     }
// // // // // TODO: this should create a watch dog to watch the information from server;
// // // //     std::vector<uint8_t> poll_and_receive() {
// // // //         if (!u_KcpClient) return {};
        
        
// // // //         auto encrypted_data = u_KcpClient->poll_and_receive();
// // // //           if(encrypted_data.size()==0)return {};


// // // //         if(!completeHandshake){
// // // //             //* this is the server public key;
// // // //             //* we extract it ;
// // // //           std::optional<std::vector<uint8_t>> result =   encryption::ecdh_step2_compute_shared_secret(encrypted_data);
// // // //           if(result.has_value()){
             
// // // //             aes_key = result.value();
// // // //             completeHandshake=true;

// // // //             // --- ADD THIS CODE TO PRINT THE KEY AS HEX ---
// // // //             std::string hex_str = "";
// // // //             for (uint8_t b : aes_key) {
// // // //                 char buf[3];
// // // //                 snprintf(buf, sizeof(buf), "%02x", b);
// // // //                 hex_str += buf;
// // // //             }
// // // //             ESP_LOGE(TAG, "Client Shared Secret (Hex): %s", hex_str.c_str());


// // // //             // ---------------------------------------------
// // // //             return {};
// // // //           }
// // // //         }
// // // //         auto Opt_decrypted_data = encryption::aes_gcm_decrypt(encrypted_data);

// // // //         if (!Opt_decrypted_data.has_value()) return {};
// // // //         return Opt_decrypted_data.value();
// // // //     }


    






// // // // // TODO: when it's first send,we don't encrypt;
// // // //     int send(const std::vector<uint8_t>& data) {

// // // //         //TODO test;
// // // // if(!wifi::isConnected()){
// // // //      ESP_LOGI(TAG, "wifi disconnected ");
// // // //             // store in disk;
// // // //          //   packageStoredFile->push_back(data);
// // // //             return -1;
// // // // }
 
// // // // //Todo test;
// // // // if( !completeHandshake){
// // // //            //ESP_LOGI(TAG, "  handshake has not finished yet");
// // // //             // store in disk;
// // // //           //  packageStoredFile->push_back(data);
// // // //             return -1;
// // // // }
 
  
// // // //   // try_send_disk_package();

// // // //          //  ESP_LOGW(TAG, " pay load size = %u bytes",data.size());
// // // //         auto Opt_encrypted_data = encryption::aes_gcm_encrypt(data,aes_key);
// // // //         if (!Opt_encrypted_data.has_value()) return -1;

// // // //         auto encrypted_data = Opt_encrypted_data.value();

// // // //                    // we encrypt here;

// // // //                    //TODO: test;
// // // //         int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
// // // //         if (r == -1) {
// // // //             ESP_LOGI(TAG, "send failed, reason: unknown");
// // // //         }
// // // //         return r;
// // // //     }
// // // //  bool send_handshakeToServer(){
// // // // if (completeHandshake) {
// // // //             ESP_LOGE(TAG, "duplicated handshake with server");
// // // //             return false;
// // // //         }

// // // //         std::optional<std::vector<uint8_t>> generated_key = encryption::ecdh_step1_generate_my_key();
// // // //         if (!generated_key.has_value()) {
// // // //             ESP_LOGE(TAG, "!generated_key.has_value()");
// // // //             return false;
// // // //         }

// // // //         const auto& pub_key_bytes = generated_key.value();
// // // //         std::vector<uint8_t> handshake_packet;
// // // //         handshake_packet.resize(sizeof(uint32_t) + pub_key_bytes.size());

// // // //         // Copy conv_id into the first 4 bytes
// // // //         std::memcpy(handshake_packet.data(), &conv_id, sizeof(uint32_t));

// // // //         // Copy public key bytes right after it
// // // //         std::memcpy(handshake_packet.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// // // //         ESP_LOGW(TAG, "ready to send to server the handshake (conv_id: %u)", conv_id);

// // // //         // FIX 2: Send the full handshake_packet instead of just the raw key
// // // //         int r = u_KcpClient->raw_udp_send(handshake_packet.data(), handshake_packet.size());
// // // //         if (r == -1) {
// // // //             ESP_LOGE(TAG, "send failed, reason: unknown");
// // // //             return false;
// // // //         }
// // // //         return true;
// // // //     }


// // // // } // namespace Network
























// // // #include "Net manager.hpp"
// // // #include "KcpClient.hpp"
// // // #include "encryption/interface.hpp"

// // // #include <mutex>
// // // #include <queue>
// // // #include <atomic>
// // // #include <cstring>

// // // #include "freertos/FreeRTOS.h"
// // // #include "freertos/task.h"
// // // #include "esp_timer.h"
// // // #include "esp_random.h"

// // // namespace Network {

// // //     // =========================================================================
// // //     // Internal State & Handles
// // //     // =========================================================================
// // //     static std::unique_ptr<KcpClient>   u_KcpClient;
// // //     static std::unique_ptr<file::FIFO>  packageStoredFile;
// // //     static uint32_t                     conv_id = 0;

// // //     // Background worker task management
// // //     static TaskHandle_t                 s_worker_task_handle = nullptr;
// // //     static std::atomic<bool>            s_worker_running{false};

// // //     // --- Ping-Pong (Double Buffering) Queues for Outbound Data ---
// // //     static std::mutex                               s_tx_mutex;
// // //     static std::vector<std::vector<uint8_t>>        incoming_queue; // Appended by producers
// // //     static std::vector<std::vector<uint8_t>>        sending_queue;  // Drained by network task

// // //     // --- Thread-Safe Rx Queue for Inbound Data ---
// // //     static std::mutex                               s_rx_mutex;
// // //     static std::queue<std::vector<uint8_t>>         rx_queue;

// // //     // Maximum RAM queue size before producers get rejected (backpressure safeguard)
// // //     static constexpr size_t MAX_RAM_QUEUE_SIZE = 128;

// // //     // Maximum disk packets to drain per loop tick (prevents choking KCP flow control)
// // //     static constexpr size_t MAX_DISK_BURST_PER_TICK = 5;

// // //     // Forward declaration of internal worker task
// // //     static void network_worker_task(void* pvParameters);

// // //     // =========================================================================
// // //     // Lifecycle Management
// // //     // =========================================================================
// // //     void init(const char* host, const char* port) {
// // //         conv_id = esp_random();

// // //         // 1. Initialize persistent storage FIFO
// // //         packageStoredFile = std::make_unique<file::FIFO>("packageStoredFile");

// // //         // 2. Initialize low-level KCP Client
// // //         u_KcpClient = std::make_unique<KcpClient>(conv_id);
// // //         u_KcpClient->initialize(host, port);

// // //         completeHandshake = false;
// // //         aes_key.clear();

// // //         // 3. Spawn dedicated network worker task (Core 1 recommended on ESP32)
// // //         if (!s_worker_running.load()) {
// // //             s_worker_running.store(true);
// // //             xTaskCreatePinnedToCore(
// // //                 network_worker_task,
// // //                 "net_worker_io",
// // //                 8192,                   // Stack size (8KB for TLS/Crypto/IO)
// // //                 nullptr,
// // //                 configMAX_PRIORITIES - 3,// High priority for latency-sensitive network I/O
// // //                 &s_worker_task_handle,
// // //                 1                       // Core 1 (separates network from Core 0 Wi-Fi stack)
// // //             );
// // //         }

// // //         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", conv_id);
// // //     }

// // //     void close_connection() {
// // //         s_worker_running.store(false);

// // //         if (s_worker_task_handle != nullptr) {
// // //             // Task will gracefully exit upon checking s_worker_running
// // //             s_worker_task_handle = nullptr;
// // //         }

// // //         if (u_KcpClient) {
// // //             u_KcpClient->close_connection();
// // //         }

// // //         ESP_LOGI(TAG, "Network connection closed");
// // //     }

// // //     // =========================================================================
// // //     // Producer Interface (Thread-Safe, Called by ANY thread)
// // //     // =========================================================================
// // //     int send(const std::vector<uint8_t>& data) {
// // //         if (data.empty()) {
// // //             return 0;
// // //         }

// // //         std::lock_guard<std::mutex> lock(s_tx_mutex);

// // //         // Protect RAM against out-of-memory if the network is disconnected for too long
// // //         if (incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
// // //             ESP_LOGW(TAG, "RAM queue full, packet dropped");
// // //             return -1;
// // //         }

// // //         // Store RAW UNENCRYPTED data.
// // //         // Encryption will take place right before transmission inside the worker task.
// // //         incoming_queue.push_back(data);
// // //         return 0;
// // //     }

// // //     std::vector<uint8_t> poll_and_receive() {
// // //         std::lock_guard<std::mutex> lock(s_rx_mutex);
// // //         if (rx_queue.empty()) {
// // //             return {};
// // //         }

// // //         std::vector<uint8_t> data = std::move(rx_queue.front());
// // //         rx_queue.pop();
// // //         return data;
// // //     }

// // //     // Retained for API compatibility. The actual KCP tick runs inside network_worker_task.
// // //     void update(uint32_t /*current_ms*/) {
// // //         // No-op: handled concurrently in network_worker_task to prevent KCP race conditions
// // //     }

// // //     bool send_handshakeToServer() {
// // //         if (completeHandshake) {
// // //             ESP_LOGE(TAG, "Duplicated handshake request rejected");
// // //             return false;
// // //         }

// // //         auto generated_key = encryption::ecdh_step1_generate_my_key();
// // //         if (!generated_key.has_value()) {
// // //             ESP_LOGE(TAG, "Failed to generate ECDH client keypair");
// // //             return false;
// // //         }

// // //         const auto& pub_key_bytes = generated_key.value();
// // //         std::vector<uint8_t> handshake_packet(sizeof(uint32_t) + pub_key_bytes.size());

// // //         // Header: 4 bytes conv_id + Client Public Key
// // //         std::memcpy(handshake_packet.data(), &conv_id, sizeof(uint32_t));
// // //         std::memcpy(handshake_packet.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// // //         ESP_LOGI(TAG, "Dispatching raw UDP handshake (conv_id: %u)", conv_id);
// // //         int r = u_KcpClient->raw_udp_send(handshake_packet.data(), handshake_packet.size());
// // //         return (r != -1);
// // //     }

// // //     // =========================================================================
// // //     // Consumer Worker Task (Single Thread: handles KCP, Flash, and Crypto)
// // //     // =========================================================================
// // //     static void network_worker_task(void* /*pvParameters*/) {
// // //         ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

// // //         while (s_worker_running.load()) {
// // //             uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

// // //             // -----------------------------------------------------------------
// // //             // 1. KCP Clock Update (MUST run in same thread as kcp_send/recv)
// // //             // -----------------------------------------------------------------
// // //             if (u_KcpClient) {
// // //                 u_KcpClient->update(now_ms);
// // //             }

// // //             // -----------------------------------------------------------------
// // //             // 2. Poll Inbound Data & Process Handshake
// // //             // -----------------------------------------------------------------
// // //             if (u_KcpClient) {
// // //                 auto incoming_data = u_KcpClient->poll_and_receive();
// // //                 if (!incoming_data.empty()) {
// // //                     if (!completeHandshake) {
// // //                         // Inbound packet during handshake phase contains Server's Public Key
// // //                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
// // //                         if (shared_secret.has_value()) {
// // //                             aes_key = shared_secret.value();
// // //                             completeHandshake = true;
// // //                             ESP_LOGI(TAG, "ECDH Shared Secret established successfully!");
// // //                         } else {
// // //                             ESP_LOGE(TAG, "Failed to compute shared secret from server packet");
// // //                         }
// // //                     } else {
// // //                         // Handshake is established: decrypt application payload
// // //                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
// // //                         if (decrypted.has_value()) {
// // //                             std::lock_guard<std::mutex> lock(s_rx_mutex);
// // //                             rx_queue.push(std::move(decrypted.value()));
// // //                         } else {
// // //                             ESP_LOGE(TAG, "Failed to decrypt incoming packet");
// // //                         }
// // //                     }
// // //                 }
// // //             }

// // //             // -----------------------------------------------------------------
// // //             // 3. Ping-Pong Swap (O(1) under brief lock)
// // //             // -----------------------------------------------------------------
// // //             {
// // //                 std::lock_guard<std::mutex> lock(s_tx_mutex);
// // //                 if (!incoming_queue.empty()) {
// // //                     sending_queue.swap(incoming_queue);
// // //                 }
// // //             }

// // //             // -----------------------------------------------------------------
// // //             // 4. State Decision: Connected vs Disconnected
// // //             // -----------------------------------------------------------------
// // //             bool is_online = wifi::isConnected() && completeHandshake;

// // //             if (!is_online) {
// // //                 // -------------------------------------------------------------
// // //                 // OFFLINE PATH: Commit pending RAM batch to persistent FIFO
// // //                 // -------------------------------------------------------------
// // //                 if (!sending_queue.empty() && packageStoredFile) {
// // //                     packageStoredFile->begin_push_mode();
// // //                     for (auto& packet : sending_queue) {
// // //                         // Persist plain data! Key is ephemeral and will change across boots.
// // //                         packageStoredFile->push_back(std::move(packet));
// // //                     }
// // //                     packageStoredFile->flush();
// // //                     sending_queue.clear();
// // //                     ESP_LOGI(TAG, "Network offline: buffered packets flushed to Flash FIFO");
// // //                 }
// // //             } else {
// // //                 // -------------------------------------------------------------
// // //                 // ONLINE PATH:
// // //                 // Step A: Drain disk FIFO first (Preserves chronological ordering)
// // //                 // -------------------------------------------------------------
// // //                 if (packageStoredFile && !packageStoredFile->empty()) {
// // //                     packageStoredFile->begin_pop_mode();
// // //                     size_t popped_count = 0;

// // //                     while (!packageStoredFile->empty() && popped_count < MAX_DISK_BURST_PER_TICK) {
// // //                         std::vector<uint8_t> disk_raw_packet = packageStoredFile->Pop();
// // //                         popped_count++;

// // //                         // Encrypt dynamically with the active session's aes_key
// // //                         auto encrypted = encryption::aes_gcm_encrypt(disk_raw_packet, aes_key);
// // //                         if (encrypted.has_value()) {
// // //                             u_KcpClient->send(encrypted->data(), encrypted->size());
// // //                         } else {
// // //                             ESP_LOGE(TAG, "Failed to encrypt disk-restored packet");
// // //                         }
// // //                     }
// // //                     packageStoredFile->flush();
// // //                 }

// // //                 // -------------------------------------------------------------
// // //                 // Step B: Send real-time RAM packets from sending_queue
// // //                 // -------------------------------------------------------------
// // //                 if (!sending_queue.empty()) {
// // //                     for (const auto& raw_packet : sending_queue) {
// // //                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, aes_key);
// // //                         if (encrypted.has_value()) {
// // //                             int res = u_KcpClient->send(encrypted->data(), encrypted->size());
// // //                             if (res == -1) {
// // //                                 ESP_LOGW(TAG, "KCP send queue buffer full");
// // //                             }
// // //                         } else {
// // //                             ESP_LOGE(TAG, "Failed to encrypt live packet");
// // //                         }
// // //                     }
// // //                     sending_queue.clear();
// // //                 }
// // //             }

// // //             // -----------------------------------------------------------------
// // //             // 5. Task Cadence (Yield 10ms for KCP tick timer resolution)
// // //             // -----------------------------------------------------------------
// // //             vTaskDelay(pdMS_TO_TICKS(10));
// // //         }

// // //         vTaskDelete(nullptr);
// // //     }

// // // } // namespace Network


















// // #include "Net manager.hpp"
// // #include "KcpClient.hpp"
// // #include "encryption/interface.hpp"

// // #include <mutex>
// // #include <queue>
// // #include <atomic>
// // #include <cstring>

// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"
// // #include "esp_timer.h"
// // #include "esp_random.h"

// // namespace Network {

// //     enum class HandshakeState {
// //         IDLE,           // Wi-Fi down or uninitialized 
// //         WAIT_ACK,       // Sent handshake, awaiting server response with retries
// //         ESTABLISHED     // Handshake completed, AES key ready for normal traffic
// //     };

// //     // Internal state variables
// //     static std::unique_ptr<KcpClient>   u_KcpClient;
// //     static std::unique_ptr<file::FIFO>  packageStoredFile;
// //     static uint32_t                     conv_id = 0;

// //     static TaskHandle_t                 s_worker_task_handle = nullptr;
// //     static std::atomic<bool>            s_worker_running{false};

// //     // Queues
// //     static std::mutex                               s_tx_mutex;
// //     static std::vector<std::vector<uint8_t>>        incoming_queue;
// //     static std::vector<std::vector<uint8_t>>        sending_queue;

// //     static std::mutex                               s_rx_mutex;
// //     static std::queue<std::vector<uint8_t>>         rx_queue;

// //     // Handshake State Tracking
// //     static HandshakeState               s_hs_state = HandshakeState::IDLE;
// //     static std::vector<uint8_t>         s_cached_handshake_pkt; // Holds fixed packet for retries
// //     static uint32_t                     s_last_handshake_attempt_ms = 0;
// //     static uint32_t                     s_handshake_retry_count = 0;

// //     // Retry configuration
// //     static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Retry every 600ms
// //     static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Cap backoff at 3s
// //     static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;
// //     static constexpr size_t   MAX_DISK_BURST_PER_TICK = 5;

// //     static void network_worker_task(void* pvParameters);
// //     static void handle_autonomous_handshake(uint32_t now_ms);

// //     // =========================================================================
// //     // Public APIs
// //     // =========================================================================
// //     void init(const char* host, const char* port) {
// //         conv_id = esp_random();

// //         packageStoredFile = std::make_unique<file::FIFO>("packageStoredFile");
// //         u_KcpClient = std::make_unique<KcpClient>(conv_id);
// //         u_KcpClient->initialize(host, port);

// //         completeHandshake = false;
// //         aes_key.clear();
// //         s_hs_state = HandshakeState::IDLE;
// //         s_cached_handshake_pkt.clear();

// //         if (!s_worker_running.load()) {
// //             s_worker_running.store(true);
// //             xTaskCreatePinnedToCore(
// //                 network_worker_task,
// //                 "net_worker_io",
// //                 8192,
// //                 nullptr,
// //                 configMAX_PRIORITIES - 3,
// //                 &s_worker_task_handle,
// //                 1
// //             );
// //         }

// //         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", conv_id);
// //     }

// //     void close_connection() {
// //         s_worker_running.store(false);
// //         s_worker_task_handle = nullptr;

// //         if (u_KcpClient) {
// //             u_KcpClient->close_connection();
// //         }
// //         s_hs_state = HandshakeState::IDLE;
// //     }

// //     int send(const std::vector<uint8_t>& data) {
// //         if (data.empty()) return 0;

// //         std::lock_guard<std::mutex> lock(s_tx_mutex);
// //         if (incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
// //             ESP_LOGW(TAG, "RAM queue full, packet dropped");
// //             return -1;
// //         }

// //         incoming_queue.push_back(data);
// //         return 0;
// //     }

// //     std::vector<uint8_t> poll_and_receive() {
// //         std::lock_guard<std::mutex> lock(s_rx_mutex);
// //         if (rx_queue.empty()) return {};

// //         std::vector<uint8_t> data = std::move(rx_queue.front());
// //         rx_queue.pop();
// //         return data;
// //     }

// //     void update(uint32_t /*current_ms*/) {
// //         // Managed internally inside the worker task
// //     }

// //     // =========================================================================
// //     // Internal Handshake Engine (Called only within Worker Thread)
// //     // =========================================================================
// //     static void handle_autonomous_handshake(uint32_t now_ms) {
// //         bool wifi_ok = wifi::isConnected();

// //         // 1. If Wi-Fi dropped, reset handshake state machine
// //         if (!wifi_ok) {
// //             if (s_hs_state != HandshakeState::IDLE) {
// //                 ESP_LOGW(TAG, "Wi-Fi lost. Resetting handshake state.");
// //                 s_hs_state = HandshakeState::IDLE;
// //                 completeHandshake = false;
// //                 s_cached_handshake_pkt.clear();
// //                 s_handshake_retry_count = 0;
// //             }
// //             return;
// //         }

// //         // 2. Wi-Fi is connected, progress through state machine
// //         switch (s_hs_state) {
// //             case HandshakeState::IDLE: {
// //                 ESP_LOGI(TAG, "Wi-Fi active. Generating ECDH keys...");
// //                 auto my_key_opt = encryption::ecdh_step1_generate_my_key();
// //                 if (!my_key_opt.has_value()) {
// //                     ESP_LOGE(TAG, "ECDH keygen failed, retrying next tick...");
// //                     return;
// //                 }

// //                 const auto& pub_key_bytes = my_key_opt.value();
// //                 s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());
                
// //                 // Pack: [conv_id (4 bytes)] + [Client Public Key]
// //                 std::memcpy(s_cached_handshake_pkt.data(), &conv_id, sizeof(uint32_t));
// //                 std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// //                 // Initial transmission
// //                 u_KcpClient->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
// //                 s_last_handshake_attempt_ms = now_ms;
// //                 s_handshake_retry_count = 1;
// //                 s_hs_state = HandshakeState::WAIT_ACK;

// //                 ESP_LOGI(TAG, "Handshake dispatched (attempt 1)");
// //                 break;
// //             }

// //             case HandshakeState::WAIT_ACK: {
// //                 // Compute retry backoff (e.g., 600ms, 1200ms, 1800ms... up to 3000ms)
// //                 uint32_t current_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
// //                 if (current_interval > MAX_HANDSHAKE_RETRY_MS) {
// //                     current_interval = MAX_HANDSHAKE_RETRY_MS;
// //                 }

// //                 if (now_ms - s_last_handshake_attempt_ms >= current_interval) {
// //                     s_handshake_retry_count++;
// //                     ESP_LOGW(TAG, "Handshake ACK timeout. Retransmitting attempt %u...", s_handshake_retry_count);
                    
// //                     // Resend the IDENTICAL packet (Never change the key!)
// //                     u_KcpClient->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
// //                     s_last_handshake_attempt_ms = now_ms;
// //                 }
// //                 break;
// //             }

// //             case HandshakeState::ESTABLISHED:
// //                 // Nothing to do, handshake is already complete
// //                 break;
// //         }
// //     }

// //     // =========================================================================
// //     // Consumer Worker Task
// //     // =========================================================================
// //     static void network_worker_task(void* /*pvParameters*/) {
// //         while (s_worker_running.load()) {
// //             uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

// //             // 1. Maintain Autonomous Handshake (Send / Retry logic)
// //             handle_autonomous_handshake(now_ms);

// //             // 2. KCP Clock Update (Always ticks in this same thread)
// //             if (u_KcpClient) {
// //                 u_KcpClient->update(now_ms);
// //             }

// //             // 3. Receive & Process Inbound Packets
// //             if (u_KcpClient) {
// //                 auto incoming_data = u_KcpClient->poll_and_receive();
// //                 if (!incoming_data.empty()) {
// //                     if (s_hs_state == HandshakeState::WAIT_ACK) {
// //                         // Received server's public key response!
// //                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
// //                         if (shared_secret.has_value()) {
// //                             aes_key = shared_secret.value();
// //                             completeHandshake = true;
// //                             s_hs_state = HandshakeState::ESTABLISHED;
// //                             s_cached_handshake_pkt.clear(); // Free memory

// //                             ESP_LOGI(TAG, "Handshake completed successfully!");
// //                         } else {
// //                             ESP_LOGE(TAG, "Corrupt server handshake response, will continue retrying");
// //                         }
// //                     } else if (s_hs_state == HandshakeState::ESTABLISHED) {
// //                         // Regular traffic: Decrypt application packet
// //                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
// //                         if (decrypted.has_value()) {
// //                             std::lock_guard<std::mutex> lock(s_rx_mutex);
// //                             rx_queue.push(std::move(decrypted.value()));
// //                         }
// //                     }
// //                 }
// //             }

// //             // 4. Ping-Pong Swap Outbound Queue
// //             {
// //                 std::lock_guard<std::mutex> lock(s_tx_mutex);
// //                 if (!incoming_queue.empty()) {
// //                     sending_queue.swap(incoming_queue);
// //                 }
// //             }

// //             // 5. Data Flow Routing
// //             bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

// //             if (!is_online) {
// //                 // If offline or handshake in progress: Buffer new RAM packets into Flash FIFO
// //                 if (!sending_queue.empty() && packageStoredFile) {
// //                     packageStoredFile->begin_push_mode();
// //                     for (auto& packet : sending_queue) {
// //                         packageStoredFile->push_back(std::move(packet)); // Plain text
// //                     }
// //                     packageStoredFile->flush();
// //                     sending_queue.clear();
// //                 }
// //             } else {
// //                 // Drain Flash FIFO first (Oldest packets first)
// //                 if (packageStoredFile && !packageStoredFile->empty()) {
// //                     packageStoredFile->begin_pop_mode();
// //                     size_t popped = 0;
// //                     while (!packageStoredFile->empty() && popped < MAX_DISK_BURST_PER_TICK) {
// //                         auto raw_disk_pkt = packageStoredFile->Pop();
// //                         popped++;

// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, aes_key);
// //                         if (encrypted.has_value()) {
// //                             u_KcpClient->send(encrypted->data(), encrypted->size());
// //                         }
// //                     }
// //                     packageStoredFile->flush();
// //                 }

// //                 // Drain live RAM queue
// //                 if (!sending_queue.empty()) {
// //                     for (const auto& raw_packet : sending_queue) {
// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, aes_key);
// //                         if (encrypted.has_value()) {
// //                             u_KcpClient->send(encrypted->data(), encrypted->size());
// //                         }
// //                     }
// //                     sending_queue.clear();
// //                 }
// //             }

// //             vTaskDelay(pdMS_TO_TICKS(10)); // 10ms KCP tick
// //         }

// //         vTaskDelete(nullptr);
// //     }

// // } // namespace Network














// // #include "Net manager.hpp"

// // // Internal Network & Storage Dependencies
// // #include "KcpClient.hpp"
// // #include "WiFiManager.hpp"
// // #include "FIFO_file.hpp"
// // #include "encryption/interface.hpp"

// // // System & FreeRTOS Headers
// // #include <mutex>
// // #include <queue>
// // #include <atomic>
// // #include <cstring>
// // #include <memory>

// // #include "esp_log.h"
// // #include "esp_timer.h"
// // #include "esp_random.h"
// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"

// // namespace Network {

// //     static const char* TAG = "NETWORK_MANAGER";

// //     // Handshake State Engine
// //     enum class HandshakeState {
// //         IDLE,           // Offline or awaiting connection
// //         WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
// //         ESTABLISHED     // Handshake complete, AES key active for live traffic
// //     };

// //     // =========================================================================
// //     // Static / Internal Handles
// //     // =========================================================================
// //     static std::unique_ptr<KcpClient>   s_kcp_client;
// //     static std::unique_ptr<file::FIFO>  s_disk_fifo;
// //     static uint32_t                     s_conv_id = 0;

// //     // Crypto & Handshake session state (Completely internal)
// //     static std::vector<uint8_t>         s_aes_key;
// //     static HandshakeState               s_hs_state = HandshakeState::IDLE;
// //     static std::vector<uint8_t>         s_cached_handshake_pkt; 
// //     static uint32_t                     s_last_handshake_attempt_ms = 0;
// //     static uint32_t                     s_handshake_retry_count = 0;

// //     // Worker Task Management
// //     static TaskHandle_t                 s_worker_task_handle = nullptr;
// //     static std::atomic<bool>            s_worker_running{false};

// //     // Ping-Pong Double-Buffering (Outbound)
// //     static std::mutex                               s_tx_mutex;
// //     static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Written by caller threads
// //     static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Processed by worker task

// //     // Inbound Rx Queue
// //     static std::mutex                               s_rx_mutex;
// //     static std::queue<std::vector<uint8_t>>         s_rx_queue;

// //     // Tuning Parameters
// //     static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Initial backoff window
// //     static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Upper backoff ceiling
// //     static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;  // RAM drop threshold
// //     static constexpr size_t   MAX_DISK_BURST_PER_TICK = 5;    // Max packets read from flash/tick

// //     // Forward declaration of internal task & helpers
// //     static void network_worker_task(void* pvParameters);
// //     static void handle_autonomous_handshake(uint32_t now_ms);

// //     // =========================================================================
// //     // Public API Implementation
// //     // =========================================================================

// //     void init(const char* host, const char* port) {
// //         s_conv_id = esp_random();

// //         // 1. Initialize persistent disk storage
// //         s_disk_fifo = std::make_unique<file::FIFO>("packageStoredFile");

// //         // 2. Initialize KCP instance
// //         s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
// //         s_kcp_client->initialize(host, port);

// //         // 3. Reset internal handshake and session keys
// //         s_aes_key.clear();
// //         s_cached_handshake_pkt.clear();
// //         s_hs_state = HandshakeState::IDLE;

// //         // 4. Spawn the single sender/update worker task (Core 1)
// //         if (!s_worker_running.load()) {
// //             s_worker_running.store(true);
// //             xTaskCreatePinnedToCore(
// //                 network_worker_task,
// //                 "net_worker_io",
// //                 8192,
// //                 nullptr,
// //                 configMAX_PRIORITIES - 3,
// //                 &s_worker_task_handle,
// //                 1
// //             );
// //         }

// //         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", s_conv_id);
// //     }

// //     void close_connection() {
// //         s_worker_running.store(false);
// //         s_worker_task_handle = nullptr;

// //         if (s_kcp_client) {
// //             s_kcp_client->close_connection();
// //         }

// //         s_hs_state = HandshakeState::IDLE;
// //         ESP_LOGI(TAG, "Network connection terminated");
// //     }

// //     int send(const std::vector<uint8_t>& data) {
// //         if (data.empty()) {
// //             return 0;
// //         }

// //         std::lock_guard<std::mutex> lock(s_tx_mutex);
// //         if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
// //             ESP_LOGW(TAG, "RAM queue full; message rejected (Backpressure)");
// //             return -1;
// //         }

// //         // Store unencrypted payload; background task will encrypt before transmission
// //         s_incoming_queue.push_back(data);
// //         return 0;
// //     }

// //     std::vector<uint8_t> poll_and_receive() {
// //         std::lock_guard<std::mutex> lock(s_rx_mutex);
// //         if (s_rx_queue.empty()) {
// //             return {};
// //         }

// //         std::vector<uint8_t> data = std::move(s_rx_queue.front());
// //         s_rx_queue.pop();
// //         return data;
// //     }

// //     // =========================================================================
// //     // Internal Autonomous Handshake Engine
// //     // =========================================================================

// //     static void handle_autonomous_handshake(uint32_t now_ms) {
// //         bool wifi_ok = wifi::isConnected();

// //         // If Wi-Fi is lost, cleanly reset the state machine
// //         if (!wifi_ok) {
// //             if (s_hs_state != HandshakeState::IDLE) {
// //                 ESP_LOGW(TAG, "Wi-Fi disconnected: Handshake reset to IDLE");
// //                 s_hs_state = HandshakeState::IDLE;
// //                 s_cached_handshake_pkt.clear();
// //                 s_handshake_retry_count = 0;
// //             }
// //             return;
// //         }

// //         // Wi-Fi is online; process state machine
// //         switch (s_hs_state) {
// //             case HandshakeState::IDLE: {
// //                 ESP_LOGI(TAG, "Wi-Fi ready: Generating ECDH local keypair...");
// //                 auto my_key_opt = encryption::ecdh_step1_generate_my_key();
// //                 if (!my_key_opt.has_value()) {
// //                     ESP_LOGE(TAG, "Key generation failed; will retry next tick");
// //                     return;
// //                 }

// //                 const auto& pub_key_bytes = my_key_opt.value();
// //                 s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());

// //                 // Build initial frame: [conv_id (4 bytes)] + [Client Public Key]
// //                 std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
// //                 std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// //                 // First UDP transmission
// //                 s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
// //                 s_last_handshake_attempt_ms = now_ms;
// //                 s_handshake_retry_count = 1;
// //                 s_hs_state = HandshakeState::WAIT_ACK;

// //                 ESP_LOGI(TAG, "Initial handshake dispatched (conv_id: %u)", s_conv_id);
// //                 break;
// //             }

// //             case HandshakeState::WAIT_ACK: {
// //                 // Compute retry window with backoff cap
// //                 uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
// //                 if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
// //                     retry_interval = MAX_HANDSHAKE_RETRY_MS;
// //                 }

// //                 if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
// //                     s_handshake_retry_count++;
// //                     ESP_LOGW(TAG, "Handshake ACK timed out; retransmitting attempt #%u...", s_handshake_retry_count);

// //                     // Retransmit the EXACT same packet (never regenerate keys on retry)
// //                     s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
// //                     s_last_handshake_attempt_ms = now_ms;
// //                 }
// //                 break;
// //             }

// //             case HandshakeState::ESTABLISHED:
// //                 // Handshake active; nothing to do
// //                 break;
// //         }
// //     }

// //     // =========================================================================
// //     // Single Consumer Task (Handles KCP, Crypto, and Disk I/O)
// //     // =========================================================================

// //     static void network_worker_task(void* /*pvParameters*/) {
// //         while (s_worker_running.load()) {
// //             uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

// //             // 1. Maintain autonomous handshake retries
// //             handle_autonomous_handshake(now_ms);

// //             // 2. KCP Clock update (Runs in the exact same thread as send/receive)
// //             if (s_kcp_client) {
// //                 s_kcp_client->update(now_ms);
// //             }

// //             // 3. Receive packets and process inbound server replies
// //             if (s_kcp_client) {
// //                 auto incoming_data = s_kcp_client->poll_and_receive();
// //                 if (!incoming_data.empty()) {
// //                     if (s_hs_state == HandshakeState::WAIT_ACK) {
// //                         // Received server's public key response
// //                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
// //                         if (shared_secret.has_value()) {
// //                             s_aes_key = shared_secret.value();
// //                             s_hs_state = HandshakeState::ESTABLISHED;
// //                             s_cached_handshake_pkt.clear();

// //                             ESP_LOGI(TAG, "Handshake completed! Session AES key ready.");
// //                         } else {
// //                             ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
// //                         }
// //                     } else if (s_hs_state == HandshakeState::ESTABLISHED) {
// //                         // Established session: decrypt payload and queue for caller
// //                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
// //                         if (decrypted.has_value()) {
// //                             std::lock_guard<std::mutex> lock(s_rx_mutex);
// //                             s_rx_queue.push(std::move(decrypted.value()));
// //                         }
// //                     }
// //                 }
// //             }

// //             // 4. Ping-Pong double-buffering swap
// //             {
// //                 std::lock_guard<std::mutex> lock(s_tx_mutex);
// //                 if (!s_incoming_queue.empty()) {
// //                     s_sending_queue.swap(s_incoming_queue);
// //                 }
// //             }

// //             // 5. Check if pipeline is ready to transmit live over the network
// //             bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

// //             if (!is_online) {
// //                 // OFFLINE PATH: Write plain data to flash FIFO (Keys change across reboots)
// //                 if (!s_sending_queue.empty() && s_disk_fifo) {
// //                     s_disk_fifo->begin_push_mode();
// //                     for (auto& packet : s_sending_queue) {
// //                         s_disk_fifo->push_back(std::move(packet));
// //                     }
// //                     s_disk_fifo->flush();
// //                     s_sending_queue.clear();
// //                 }
// //             } else {
// //                 // ONLINE PATH:
// //                 // Step A: Drain backlog from disk FIFO first (Preserve packet sequence)
// //                 if (s_disk_fifo && !s_disk_fifo->empty()) {
// //                     s_disk_fifo->begin_pop_mode();
// //                     size_t popped = 0;
// //                     while (!s_disk_fifo->empty() && popped < MAX_DISK_BURST_PER_TICK) {
// //                         auto raw_disk_pkt = s_disk_fifo->Pop();
// //                         popped++;

// //                         // Encrypt on-the-fly with the active session's key
// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, s_aes_key);
// //                         if (encrypted.has_value()) {
// //                             s_kcp_client->send(encrypted->data(), encrypted->size());
// //                         }
// //                     }
// //                     s_disk_fifo->flush();
// //                 }

// //                 // Step B: Send pending RAM queue
// //                 if (!s_sending_queue.empty()) {
// //                     for (const auto& raw_packet : s_sending_queue) {
// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
// //                         if (encrypted.has_value()) {
// //                             s_kcp_client->send(encrypted->data(), encrypted->size());
// //                         }
// //                     }
// //                     s_sending_queue.clear();
// //                 }
// //             }

// //             // Yield 10ms for KCP tick interval
// //             vTaskDelay(pdMS_TO_TICKS(10));
// //         }

// //         vTaskDelete(nullptr);
// //     }

// // } // namespace Network
































































// // #include "Net manager.hpp"

// // // Internal Network & Storage Dependencies
// // #include "KcpClient.hpp"
// // #include "WiFiManager.hpp"
// // #include "FIFO_file.hpp"
// // #include "encryption/interface.hpp"

// // // System & FreeRTOS Headers
// // #include <mutex>
// // #include <queue>
// // #include <atomic>
// // #include <cstring>
// // #include <memory>

// // #include "esp_log.h"
// // #include "esp_timer.h"
// // #include "esp_random.h"
// // #include "esp_task_wdt.h" // For feeding TWDT safely
// // #include "freertos/FreeRTOS.h"
// // #include "freertos/task.h"

// // namespace Network {

// //     static const char* TAG = "NETWORK_MANAGER";

// //     // Handshake State Engine
// //     enum class HandshakeState {
// //         IDLE,           // Offline or awaiting connection
// //         WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
// //         ESTABLISHED     // Handshake complete, AES key active for live traffic
// //     };

// //     // =========================================================================
// //     // Static / Internal Handles
// //     // =========================================================================
// //     static std::unique_ptr<KcpClient>   s_kcp_client;
// //     static std::unique_ptr<file::FIFO>  s_disk_fifo;
// //     static uint32_t                     s_conv_id = 0;

// //     // Crypto & Handshake session state (Completely internal)
// //     static std::vector<uint8_t>         s_aes_key;
// //     static HandshakeState               s_hs_state = HandshakeState::IDLE;
// //     static std::vector<uint8_t>         s_cached_handshake_pkt; 
// //     static uint32_t                     s_last_handshake_attempt_ms = 0;
// //     static uint32_t                     s_handshake_retry_count = 0;

// //     // Worker Task Management
// //     static TaskHandle_t                 s_worker_task_handle = nullptr;
// //     static std::atomic<bool>            s_worker_running{false};

// //     // Ping-Pong Double-Buffering (Outbound)
// //     static std::mutex                               s_tx_mutex;
// //     static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Written by caller threads
// //     static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Processed by worker task

// //     // Inbound Rx Queue
// //     static std::mutex                               s_rx_mutex;
// //     static std::queue<std::vector<uint8_t>>         s_rx_queue;

// //     // Tuning Parameters
// //     static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Initial backoff window
// //     static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Upper backoff ceiling
// //     static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;  // RAM drop threshold

// //     // HIGH-SPEED DRAIN CONFIGURATION
// //     // 5ms CPU time budget per cycle: drains ~50-80 packets per tick without tripping watchdog
// //     static constexpr int64_t  MAX_DRAIN_BUDGET_US     = 5000; 

// //     // Forward declaration of internal task & helpers
// //     static void network_worker_task(void* pvParameters);
// //     static void handle_autonomous_handshake(uint32_t now_ms);

// //     // =========================================================================
// //     // Public API Implementation
// //     // =========================================================================

// //     void init(const char* host, const char* port) {
// //         s_conv_id = esp_random();

// //         // 1. Initialize persistent disk storage
// //         s_disk_fifo = std::make_unique<file::FIFO>("packageStoredFile");

// //         // 2. Initialize KCP instance
// //         s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
// //         s_kcp_client->initialize(host, port);

// //         // 3. Reset internal handshake and session keys
// //         s_aes_key.clear();
// //         s_cached_handshake_pkt.clear();
// //         s_hs_state = HandshakeState::IDLE;

// //         // 4. Spawn the single sender/update worker task (Core 1)
// //         if (!s_worker_running.load()) {
// //             s_worker_running.store(true);
// //             xTaskCreatePinnedToCore(
// //                 network_worker_task,
// //                 "net_worker_io",
// //                 8192,
// //                 nullptr,
// //                 configMAX_PRIORITIES - 3,
// //                 &s_worker_task_handle,
// //                 1
// //             );
// //         }

// //         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", s_conv_id);
// //     }

// //     void close_connection() {
// //         s_worker_running.store(false);
// //         s_worker_task_handle = nullptr;

// //         if (s_kcp_client) {
// //             s_kcp_client->close_connection();
// //         }

// //         s_hs_state = HandshakeState::IDLE;
// //         ESP_LOGI(TAG, "Network connection terminated");
// //     }

// //     int send(const std::vector<uint8_t>& data) {
// //         if (data.empty()) {
// //             return 0;
// //         }

// //         std::lock_guard<std::mutex> lock(s_tx_mutex);
// //         if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
// //             ESP_LOGW(TAG, "RAM queue full; message rejected (Backpressure)");
// //             return -1;
// //         }

// //         // Store unencrypted payload; background task will encrypt before transmission
// //         s_incoming_queue.push_back(data);
// //         return 0;
// //     }

// //     std::vector<uint8_t> poll_and_receive() {
// //         std::lock_guard<std::mutex> lock(s_rx_mutex);
// //         if (s_rx_queue.empty()) {
// //             return {};
// //         }

// //         std::vector<uint8_t> data = std::move(s_rx_queue.front());
// //         s_rx_queue.pop();
// //         return data;
// //     }

// //     // =========================================================================
// //     // Internal Autonomous Handshake Engine
// //     // =========================================================================

// //     static void handle_autonomous_handshake(uint32_t now_ms) {
// //         bool wifi_ok = wifi::isConnected();

// //         // If Wi-Fi is lost, cleanly reset the state machine
// //         if (!wifi_ok) {
// //             if (s_hs_state != HandshakeState::IDLE) {
// //                 ESP_LOGW(TAG, "Wi-Fi disconnected: Handshake reset to IDLE");
// //                 s_hs_state = HandshakeState::IDLE;
// //                 s_cached_handshake_pkt.clear();
// //                 s_handshake_retry_count = 0;
// //             }
// //             return;
// //         }

// //         // Wi-Fi is online; process state machine
// //         switch (s_hs_state) {
// //             case HandshakeState::IDLE: {
// //                 ESP_LOGI(TAG, "Wi-Fi ready: Generating ECDH local keypair...");
// //                 auto my_key_opt = encryption::ecdh_step1_generate_my_key();
// //                 if (!my_key_opt.has_value()) {
// //                     ESP_LOGE(TAG, "Key generation failed; will retry next tick");
// //                     return;
// //                 }

// //                 const auto& pub_key_bytes = my_key_opt.value();
// //                 s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());

// //                 // Build initial frame: [conv_id (4 bytes)] + [Client Public Key]
// //                 std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
// //                 std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// //                 // First UDP transmission
// //                 s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
// //                 s_last_handshake_attempt_ms = now_ms;
// //                 s_handshake_retry_count = 1;
// //                 s_hs_state = HandshakeState::WAIT_ACK;

// //                 ESP_LOGI(TAG, "Initial handshake dispatched (conv_id: %u)", s_conv_id);
// //                 break;
// //             }

// //             case HandshakeState::WAIT_ACK: {
// //                 uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
// //                 if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
// //                     retry_interval = MAX_HANDSHAKE_RETRY_MS;
// //                 }

// //                 if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
// //                     s_handshake_retry_count++;
// //                     ESP_LOGW(TAG, "Handshake ACK timed out; retransmitting attempt #%u...", s_handshake_retry_count);

// //                     s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
// //                     s_last_handshake_attempt_ms = now_ms;
// //                 }
// //                 break;
// //             }

// //             case HandshakeState::ESTABLISHED:
// //                 break;
// //         }
// //     }

// //     // =========================================================================
// //     // Single Consumer Task (Optimized High-Speed Drain)
// //     // =========================================================================

// //     // static void network_worker_task(void* /*pvParameters*/) {
// //     //     while (s_worker_running.load()) {
// //     //         uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

// //     //         // 1. Maintain autonomous handshake retries
// //     //         handle_autonomous_handshake(now_ms);

// //     //         // 2. KCP Clock update (Runs in the exact same thread as send/receive)
// //     //         if (s_kcp_client) {
// //     //             s_kcp_client->update(now_ms);
// //     //         }

// //     //         // 3. Receive packets and process inbound server replies
// //     //         if (s_kcp_client) {
// //     //             auto incoming_data = s_kcp_client->poll_and_receive();
// //     //             if (!incoming_data.empty()) {
// //     //                 if (s_hs_state == HandshakeState::WAIT_ACK) {
// //     //                     auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
// //     //                     if (shared_secret.has_value()) {
// //     //                         s_aes_key = shared_secret.value();
// //     //                         s_hs_state = HandshakeState::ESTABLISHED;
// //     //                         s_cached_handshake_pkt.clear();

// //     //                         ESP_LOGI(TAG, "Handshake completed! Session AES key ready.");
// //     //                     } else {
// //     //                         ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
// //     //                     }
// //     //                 } else if (s_hs_state == HandshakeState::ESTABLISHED) {
// //     //                     auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
// //     //                     if (decrypted.has_value()) {
// //     //                         std::lock_guard<std::mutex> lock(s_rx_mutex);
// //     //                         s_rx_queue.push(std::move(decrypted.value()));
// //     //                     }
// //     //                 }
// //     //             }
// //     //         }

// //     //         // 4. Ping-Pong double-buffering swap
// //     //         {
// //     //             std::lock_guard<std::mutex> lock(s_tx_mutex);
// //     //             if (!s_incoming_queue.empty()) {
// //     //                 s_sending_queue.swap(s_incoming_queue);
// //     //             }
// //     //         }

// //     //         // 5. High-Performance Drain Pipeline
// //     //         bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);
// //     //         bool is_draining_disk = false;

// //     //         if (!is_online) {
// //     //             // OFFLINE PATH: Write unencrypted data to Flash FIFO
// //     //             if (!s_sending_queue.empty() && s_disk_fifo) {
// //     //                 s_disk_fifo->begin_push_mode();
// //     //                 for (auto& packet : s_sending_queue) {
// //     //                     s_disk_fifo->push_back(std::move(packet));
// //     //                 }
// //     //                 s_disk_fifo->flush();
// //     //                 s_sending_queue.clear();
// //     //             }
// //     //         } else {
// //     //             // ONLINE PATH: Prioritize draining historical backlog at full wire speed
// //     //             if (s_disk_fifo && !s_disk_fifo->empty()) {
// //     //                 is_draining_disk = true;

// //     //                 // Step A: Append fresh RAM packets to disk TAIL to preserve sequence
// //     //                 if (!s_sending_queue.empty()) {
// //     //                     s_disk_fifo->begin_push_mode();
// //     //                     for (auto& packet : s_sending_queue) {
// //     //                         s_disk_fifo->push_back(std::move(packet));
// //     //                     }
// //     //                     s_sending_queue.clear();
// //     //                 }

// //     //                 // Step B: High-speed time-budgeted burst from disk HEAD
// //     //                 s_disk_fifo->begin_pop_mode();
// //     //                 int64_t drain_start_us = esp_timer_get_time();
// //     //                 size_t popped_count = 0;

// //     //                 while (!s_disk_fifo->empty()) {
// //     //                     // Watchdog Protection: Do not block CPU longer than 5ms
// //     //                     if (esp_timer_get_time() - drain_start_us > MAX_DRAIN_BUDGET_US) {
// //     //                         break;
// //     //                     }

// //     //                     auto raw_disk_pkt = s_disk_fifo->Pop();
// //     //                     auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, s_aes_key);
                        
// //     //                     if (encrypted.has_value()) {
// //     //                         int res = s_kcp_client->send(encrypted->data(), encrypted->size());
// //     //                         if (res == -1) {
// //     //                             // KCP snd_queue / socket is saturated. Yield and let NIC push bytes!
// //     //                             break;
// //     //                         }
// //     //                     }
// //     //                     popped_count++;
// //     //                 }

// //     //                 // Flush updated disk read pointer once per burst (not per packet!)
// //     //                 s_disk_fifo->flush();
// //     //             } 
// //     //             // Disk is 100% empty: Direct zero-copy transmission for live stream
// //     //             else if (!s_sending_queue.empty()) {
// //     //                 for (const auto& raw_packet : s_sending_queue) {
// //     //                     auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
// //     //                     if (encrypted.has_value()) {
// //     //                         s_kcp_client->send(encrypted->data(), encrypted->size());
// //     //                     }
// //     //                 }
// //     //                 s_sending_queue.clear();
// //     //             }
// //     //         }

// //     //         // 6. Dynamic Cadence / Yielding
// //     //         // While actively catching up from flash, yield for only 1 tick (1ms) to maximize throughput.
// //     //         // When steady-state or offline, yield 10ms to conserve CPU and power.
// //     //         if (is_draining_disk && s_disk_fifo && !s_disk_fifo->empty()) {
// //     //             vTaskDelay(1); 
// //     //         } else {
// //     //             vTaskDelay(pdMS_TO_TICKS(10));
// //     //         }
// //     //     }

// //     //     vTaskDelete(nullptr);
// //     // }
// // static void network_worker_task(void* /*pvParameters*/) {
// //         // --- Profiling / Diagnostic Counters (Reset every 1 second) ---
// //         int64_t perf_log_timer_us = esp_timer_get_time();
        
// //         uint32_t stat_loop_count = 0;
// //         int64_t  stat_total_loop_us = 0;
        
// //         // Time spent in stages (microseconds)
// //         int64_t  stat_kcp_update_us = 0;
// //         int64_t  stat_kcp_recv_us = 0;
// //         int64_t  stat_fifo_push_us = 0;
// //         int64_t  stat_fifo_pop_us = 0;
// //         int64_t  stat_crypto_us = 0;
// //         int64_t  stat_kcp_send_us = 0;
// //         int64_t  stat_fifo_flush_us = 0;
// //         int64_t  stat_fifo_mode_switch_us = 0;
        
// //         uint32_t stat_disk_pkts_popped = 0;
// //         uint32_t stat_live_pkts_sent = 0;

// //         while (s_worker_running.load()) {
// //             int64_t loop_start_us = esp_timer_get_time();
// //             uint32_t now_ms = static_cast<uint32_t>(loop_start_us / 1000ULL);

// //             // 1. Autonomous Handshake
// //             handle_autonomous_handshake(now_ms);

// //             // 2. KCP Clock update
// //             if (s_kcp_client) {
// //                 int64_t t0 = esp_timer_get_time();
// //                 s_kcp_client->update(now_ms);
// //                 stat_kcp_update_us += (esp_timer_get_time() - t0);
// //             }

// //             // 3. Receive & Handshake/Data
// //             if (s_kcp_client) {
// //                 int64_t t0 = esp_timer_get_time();
// //                 auto incoming_data = s_kcp_client->poll_and_receive();
// //                 stat_kcp_recv_us += (esp_timer_get_time() - t0);

// //                 if (!incoming_data.empty()) {
// //                     if (s_hs_state == HandshakeState::WAIT_ACK) {
// //                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
// //                         if (shared_secret.has_value()) {
// //                             s_aes_key = shared_secret.value();
// //                             s_hs_state = HandshakeState::ESTABLISHED;
// //                             s_cached_handshake_pkt.clear();
// //                             ESP_LOGI(TAG, "Handshake completed! Session AES key ready.");
// //                         } else {
// //                             ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
// //                         }
// //                     } else if (s_hs_state == HandshakeState::ESTABLISHED) {
// //                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
// //                         if (decrypted.has_value()) {
// //                             std::lock_guard<std::mutex> lock(s_rx_mutex);
// //                             s_rx_queue.push(std::move(decrypted.value()));
// //                         }
// //                     }
// //                 }
// //             }

// //             // 4. Ping-Pong double-buffering swap
// //             {
// //                 std::lock_guard<std::mutex> lock(s_tx_mutex);
// //                 if (!s_incoming_queue.empty()) {
// //                     s_sending_queue.swap(s_incoming_queue);
// //                 }
// //             }

// //             // 5. High-Performance Drain Pipeline
// //             bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);
// //             bool is_draining_disk = false;

// //             if (!is_online) {
// //                 // OFFLINE PATH: Write unencrypted data to Flash FIFO
// //                 if (!s_sending_queue.empty() && s_disk_fifo) {
// //                     int64_t t0 = esp_timer_get_time();
// //                     s_disk_fifo->begin_push_mode();
// //                     for (auto& packet : s_sending_queue) {
// //                         s_disk_fifo->push_back(std::move(packet));
// //                     }
// //                     stat_fifo_push_us += (esp_timer_get_time() - t0);

// //                     int64_t tf = esp_timer_get_time();
// //                     s_disk_fifo->flush();
// //                     stat_fifo_flush_us += (esp_timer_get_time() - tf);

// //                     s_sending_queue.clear();
// //                 }
// //             } else {
// //                 // ONLINE PATH:
// //                 if (s_disk_fifo && !s_disk_fifo->empty()) {
// //                     is_draining_disk = true;

// //                     // Step A: Append fresh RAM packets to disk TAIL
// //                     if (!s_sending_queue.empty()) {
// //                         int64_t tm = esp_timer_get_time();
// //                         s_disk_fifo->begin_push_mode();
// //                         stat_fifo_mode_switch_us += (esp_timer_get_time() - tm);

// //                         int64_t tp = esp_timer_get_time();
// //                         for (auto& packet : s_sending_queue) {
// //                             s_disk_fifo->push_back(std::move(packet));
// //                         }
// //                         stat_fifo_push_us += (esp_timer_get_time() - tp);

// //                         s_sending_queue.clear();
// //                     }

// //                     // Step B: High-speed burst from disk HEAD
// //                     int64_t tm = esp_timer_get_time();
// //                     s_disk_fifo->begin_pop_mode();
// //                     stat_fifo_mode_switch_us += (esp_timer_get_time() - tm);

// //                     int64_t drain_start_us = esp_timer_get_time();

// //                     while (!s_disk_fifo->empty()) {
// //                         if (esp_timer_get_time() - drain_start_us > MAX_DRAIN_BUDGET_US) {
// //                             break;
// //                         }

// //                         // Measure Pop time
// //                         int64_t t_pop = esp_timer_get_time();
// //                         auto raw_disk_pkt = s_disk_fifo->Pop();
// //                         stat_fifo_pop_us += (esp_timer_get_time() - t_pop);

// //                         // Measure AES-GCM Encrypt time
// //                         int64_t t_crypto = esp_timer_get_time();
// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, s_aes_key);
// //                         stat_crypto_us += (esp_timer_get_time() - t_crypto);
                        
// //                         // Measure KCP Send time
// //                         if (encrypted.has_value()) {
// //                             int64_t t_send = esp_timer_get_time();
// //                             int res = s_kcp_client->send(encrypted->data(), encrypted->size());
// //                             stat_kcp_send_us += (esp_timer_get_time() - t_send);

// //                             if (res == -1) {
// //                                 // Buffer full
// //                                 break;
// //                             }
// //                         }
// //                         stat_disk_pkts_popped++;
// //                     }

// //                     // Measure Flash Commit/Flush time
// //                     int64_t tf = esp_timer_get_time();
// //                     s_disk_fifo->flush();
// //                     stat_fifo_flush_us += (esp_timer_get_time() - tf);

// //                 } 
// //                 // Disk is empty: direct live stream
// //                 else if (!s_sending_queue.empty()) {
// //                     for (const auto& raw_packet : s_sending_queue) {
// //                         int64_t t_crypto = esp_timer_get_time();
// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
// //                         stat_crypto_us += (esp_timer_get_time() - t_crypto);

// //                         if (encrypted.has_value()) {
// //                             int64_t t_send = esp_timer_get_time();
// //                             s_kcp_client->send(encrypted->data(), encrypted->size());
// //                             stat_kcp_send_us += (esp_timer_get_time() - t_send);
// //                         }
// //                         stat_live_pkts_sent++;
// //                     }
// //                     s_sending_queue.clear();
// //                 }
// //             }

// //             // Record total iteration time
// //             int64_t loop_elapsed_us = esp_timer_get_time() - loop_start_us;
// //             stat_total_loop_us += loop_elapsed_us;
// //             stat_loop_count++;

// //             // =================================================================
// //             // PRINT DIAGNOSTIC BENCHMARK EVERY 1 SECOND
// //             // =================================================================
// //             if (esp_timer_get_time() - perf_log_timer_us >= 1000000ULL) {
// //                 if (is_draining_disk || stat_disk_pkts_popped > 0) {
// //                     ESP_LOGW(TAG, "=== DRAIN PERF: Popped: %u pkts | Live: %u pkts | Loops: %u ===", 
// //                              stat_disk_pkts_popped, stat_live_pkts_sent, stat_loop_count);
// //                     ESP_LOGW(TAG, "  Mode Switch:  %lld us", stat_fifo_mode_switch_us);
// //                     ESP_LOGW(TAG, "  FIFO Pop:     %lld us", stat_fifo_pop_us);
// //                     ESP_LOGW(TAG, "  FIFO Push:    %lld us", stat_fifo_push_us);
// //                     ESP_LOGW(TAG, "  FIFO Flush:   %lld us", stat_fifo_flush_us);
// //                     ESP_LOGW(TAG, "  AES-GCM:      %lld us", stat_crypto_us);
// //                     ESP_LOGW(TAG, "  KCP Send:     %lld us", stat_kcp_send_us);
// //                     ESP_LOGW(TAG, "  Total Loop:   %lld us", stat_total_loop_us);
// //                 }

// //                 // Reset counters
// //                 stat_loop_count = 0;
// //                 stat_total_loop_us = 0;
// //                 stat_kcp_update_us = 0;
// //                 stat_kcp_recv_us = 0;
// //                 stat_fifo_push_us = 0;
// //                 stat_fifo_pop_us = 0;
// //                 stat_crypto_us = 0;
// //                 stat_kcp_send_us = 0;
// //                 stat_fifo_flush_us = 0;
// //                 stat_fifo_mode_switch_us = 0;
// //                 stat_disk_pkts_popped = 0;
// //                 stat_live_pkts_sent = 0;
// //                 perf_log_timer_us = esp_timer_get_time();
// //             }

// //             // Yield
// //             if (is_draining_disk && s_disk_fifo && !s_disk_fifo->empty()) {
// //                 vTaskDelay(1); 
// //             } else {
// //                 vTaskDelay(pdMS_TO_TICKS(10));
// //             }
// //         }

// //         vTaskDelete(nullptr);
// //     }
// // } // namespace Network




























// #include "Net manager.hpp"

// // Internal Network & Storage Dependencies
// #include "KcpClient.hpp"
// #include "WiFiManager.hpp"
// #include "FIFO_file.hpp"
// #include "encryption/interface.hpp"

// // System & FreeRTOS Headers
// #include <mutex>
// #include <queue>
// #include <atomic>
// #include <cstring>
// #include <memory>
// #include <vector>

// #include "esp_log.h"
// #include "esp_timer.h"
// #include "esp_random.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// namespace Network {

//     static const char* TAG = "NETWORK_MANAGER";

//     // Handshake State Engine
//     enum class HandshakeState {
//         IDLE,           // Offline or awaiting connection
//         WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
//         ESTABLISHED     // Handshake complete, AES key active for live traffic
//     };

//     // =========================================================================
//     // Static / Internal Handles
//     // =========================================================================
//     static std::unique_ptr<KcpClient>   s_kcp_client;
//     static std::unique_ptr<file::FIFO>  s_disk_fifo;
//     static uint32_t                     s_conv_id = 0;

//     // Crypto & Handshake session state
//     static std::vector<uint8_t>         s_aes_key;
//     static HandshakeState               s_hs_state = HandshakeState::IDLE;
//     static std::vector<uint8_t>         s_cached_handshake_pkt; 
//     static uint32_t                     s_last_handshake_attempt_ms = 0;
//     static uint32_t                     s_handshake_retry_count = 0;

//     // Worker Task Management
//     static TaskHandle_t                 s_worker_task_handle = nullptr;
//     static std::atomic<bool>            s_worker_running{false};

//     // Ping-Pong Double-Buffering (Outbound)
//     static std::mutex                               s_tx_mutex;
//     static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Written by caller threads
//     static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Processed by worker task

//     // Inbound Rx Queue
//     static std::mutex                               s_rx_mutex;
//     static std::queue<std::vector<uint8_t>>         s_rx_queue;

//     // Tuning Parameters
//     static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Initial backoff window
//     static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Upper backoff ceiling
//     static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;  // Producer drop threshold
//     static constexpr size_t   DRAIN_BURST_PER_TICK    = 25;   // Packets popped per tick without blocking
//     static constexpr size_t   MAX_LIVE_RAM_BUFFER     = 256;  // Safeguard cap for interim RAM backlog

//     // Forward declaration of internal task & helpers
//     static void network_worker_task(void* pvParameters);
//     static void handle_autonomous_handshake(uint32_t now_ms);

//     // =========================================================================
//     // Public API Implementation
//     // =========================================================================

//     void init(const char* host, const char* port) {
//         s_conv_id = esp_random();

//         // 1. Initialize persistent disk storage
//         s_disk_fifo = std::make_unique<file::FIFO>("packageStoredFile");

//         // 2. Initialize KCP instance
//         s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
//         s_kcp_client->initialize(host, port);

//         // 3. Reset internal handshake and session keys
//         s_aes_key.clear();
//         s_cached_handshake_pkt.clear();
//         s_hs_state = HandshakeState::IDLE;
//         s_handshake_retry_count = 0;

//         // 4. Spawn the single sender/update worker task (Core 1)
//         if (!s_worker_running.load()) {
//             s_worker_running.store(true);
//             xTaskCreatePinnedToCore(
//                 network_worker_task,
//                 "net_worker_io",
//                 8192,
//                 nullptr,
//                 configMAX_PRIORITIES - 3,
//                 &s_worker_task_handle,
//                 1
//             );
//         }

//         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", s_conv_id);
//     }

//     void close_connection() {
//         s_worker_running.store(false);
//         s_worker_task_handle = nullptr;

//         if (s_kcp_client) {
//             s_kcp_client->close_connection();
//         }

//         s_hs_state = HandshakeState::IDLE;
//         ESP_LOGI(TAG, "Network connection terminated");
//     }

//     int send(const std::vector<uint8_t>& data) {
//         if (data.empty()) {
//             return 0;
//         }

//         std::lock_guard<std::mutex> lock(s_tx_mutex);
//         if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
//             ESP_LOGW(TAG, "RAM queue full; message rejected (Backpressure)");
//             return -1;
//         }

//         // Store unencrypted payload; background task encrypts immediately before socket send
//         s_incoming_queue.push_back(data);
//         return 0;
//     }

//     std::vector<uint8_t> poll_and_receive() {
//         std::lock_guard<std::mutex> lock(s_rx_mutex);
//         if (s_rx_queue.empty()) {
//             return {};
//         }

//         std::vector<uint8_t> data = std::move(s_rx_queue.front());
//         s_rx_queue.pop();
//         return data;
//     }

//     // =========================================================================
//     // Internal Autonomous Handshake Engine
//     // =========================================================================

//     static void handle_autonomous_handshake(uint32_t now_ms) {
//         bool wifi_ok = wifi::isConnected();

//         // If Wi-Fi is lost, cleanly reset the state machine
//         if (!wifi_ok) {
//             if (s_hs_state != HandshakeState::IDLE) {
//                 ESP_LOGW(TAG, "Wi-Fi disconnected: Handshake reset to IDLE");
//                 s_hs_state = HandshakeState::IDLE;
//                 s_cached_handshake_pkt.clear();
//                 s_handshake_retry_count = 0;
//             }
//             return;
//         }

//         // Wi-Fi is online; process state machine
//         switch (s_hs_state) {
//             case HandshakeState::IDLE: {
//                 ESP_LOGI(TAG, "Wi-Fi ready: Generating ECDH local keypair...");
//                 auto my_key_opt = encryption::ecdh_step1_generate_my_key();
//                 if (!my_key_opt.has_value()) {
//                     ESP_LOGE(TAG, "Key generation failed; will retry next tick");
//                     return;
//                 }

//                 const auto& pub_key_bytes = my_key_opt.value();
//                 s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());

//                 // Build initial frame: [conv_id (4 bytes)] + [Client Public Key]
//                 std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
//                 std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

//                 // First UDP transmission
//                 s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
//                 s_last_handshake_attempt_ms = now_ms;
//                 s_handshake_retry_count = 1;
//                 s_hs_state = HandshakeState::WAIT_ACK;

//                 ESP_LOGI(TAG, "Initial handshake dispatched (conv_id: %u)", s_conv_id);
//                 break;
//             }

//             case HandshakeState::WAIT_ACK: {
//                 uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
//                 if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
//                     retry_interval = MAX_HANDSHAKE_RETRY_MS;
//                 }

//                 if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
//                     s_handshake_retry_count++;
//                     ESP_LOGW(TAG, "Handshake ACK timed out; retransmitting attempt #%u...", s_handshake_retry_count);

//                     s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
//                     s_last_handshake_attempt_ms = now_ms;
//                 }
//                 break;
//             }

//             case HandshakeState::ESTABLISHED:
//                 break;
//         }
//     }

//     // =========================================================================
//     // Single Consumer Task (Zero-Thrashing Architecture)
//     // =========================================================================

//     static void network_worker_task(void* /*pvParameters*/) {
//         // [NON-STATIC LOCAL VECTOR]: Lives on this task's frame across loop iterations.
//         // Cleanly destructed if the task terminates; zero static side-effects across init/reinit.
//         std::vector<std::vector<uint8_t>> live_ram_buffer;
//         live_ram_buffer.reserve(32);

//         while (s_worker_running.load()) {
//             uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

//             // 1. Maintain autonomous handshake retries
//             handle_autonomous_handshake(now_ms);

//             // 2. KCP Clock update (Runs in the exact same thread as send/receive)
//             if (s_kcp_client) {
//                 s_kcp_client->update(now_ms);
//             }

//             // 3. Receive packets and process inbound server replies
//             if (s_kcp_client) {
//                 auto incoming_data = s_kcp_client->poll_and_receive();
//                 if (!incoming_data.empty()) {
//                     if (s_hs_state == HandshakeState::WAIT_ACK) {
//                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
//                         if (shared_secret.has_value()) {
//                             s_aes_key = shared_secret.value();
//                             s_hs_state = HandshakeState::ESTABLISHED;
//                             s_cached_handshake_pkt.clear();

//                             ESP_LOGI(TAG, "Handshake completed! Session AES key ready.");
//                         } else {
//                             ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
//                         }
//                     } else if (s_hs_state == HandshakeState::ESTABLISHED) {
//                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
//                         if (decrypted.has_value()) {
//                             std::lock_guard<std::mutex> lock(s_rx_mutex);
//                             s_rx_queue.push(std::move(decrypted.value()));
//                         }
//                     }
//                 }
//             }

//             // 4. Ping-Pong double-buffering swap
//             {
//                 std::lock_guard<std::mutex> lock(s_tx_mutex);
//                 if (!s_incoming_queue.empty()) {
//                     s_sending_queue.swap(s_incoming_queue);
//                 }
//             }

//             // 5. Zero-Thrashing Pipeline
//             bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

//             if (!is_online) {
//                 // =============================================================
//                 // OFFLINE PATH: Wi-Fi is down or handshaking.
//                 // Flush RAM buffers to persistent Flash FIFO.
//                 // =============================================================
                
//                 // Flush leftover live RAM buffer first
//                 if (!live_ram_buffer.empty() && s_disk_fifo) {
//                   //  s_disk_fifo->begin_push_mode();
//                     for (auto& pkt : live_ram_buffer) {
//                         s_disk_fifo->push_back(std::move(pkt));
//                     }
//                     live_ram_buffer.clear();
//                 }

//                 // Push current batch from producers
//                 if (!s_sending_queue.empty() && s_disk_fifo) {
//                    // s_disk_fifo->begin_push_mode();
//                     for (auto& pkt : s_sending_queue) {
//                         s_disk_fifo->push_back(std::move(pkt));
//                     }
//                     s_disk_fifo->flush();
//                     s_sending_queue.clear();
//                 }
//             } else {
//                 // =============================================================
//                 // ONLINE PATH: Zero Mode Switching While Draining
//                 // =============================================================

//                 // Stash newly arrived live packets into RAM (NEVER touch flash!)
//                 if (!s_sending_queue.empty()) {
//                     for (auto& pkt : s_sending_queue) {
//                         if (live_ram_buffer.size() < MAX_LIVE_RAM_BUFFER) {
//                             live_ram_buffer.push_back(std::move(pkt));
//                         } else {
//                             ESP_LOGW(TAG, "Live RAM buffer saturated during catch-up, dropping packet");
//                         }
//                     }
//                     s_sending_queue.clear();
//                 }

//                 // PHASE 1: Disk has historical backlog -> Drain strictly in POP mode
//                 if (s_disk_fifo && !s_disk_fifo->empty()) {
//                     // Stay continuously in pop mode without any begin_push_mode() calls
//                    // s_disk_fifo->begin_pop_mode();

//                     size_t popped_count = 0;
//                     while (!s_disk_fifo->empty() && popped_count < DRAIN_BURST_PER_TICK) {
//                         auto raw_disk_pkt = s_disk_fifo->Pop();
//                         if (raw_disk_pkt.empty()) {
//                             break;
//                         }

//                         auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, s_aes_key);
//                         if (encrypted.has_value()) {
//                             int res = s_kcp_client->send(encrypted->data(), encrypted->size());
//                             if (res == -1) {
//                                 // KCP socket buffer saturated; pause pop and wait for ACKs
//                                 break;
//                             }
//                         }
//                         popped_count++;
//                     }

//                     // Once the disk is completely empty, commit final pointer and truncate to 8 bytes
//                     if (s_disk_fifo->empty()) {
//                         s_disk_fifo->flush();
//                         ESP_LOGI(TAG, ">>> Disk backlog 100%% drained! <<<");
//                     }
//                 } 
//                 // PHASE 2: Disk is 100% empty -> Drain the accumulated RAM queue directly
//                 else if (!live_ram_buffer.empty()) {
//                     for (const auto& raw_pkt : live_ram_buffer) {
//                         auto encrypted = encryption::aes_gcm_encrypt(raw_pkt, s_aes_key);
//                         if (encrypted.has_value()) {
//                             int res = s_kcp_client->send(encrypted->data(), encrypted->size());
//                             if (res == -1) {
//                                 // Network saturated; keep remaining packets in live_ram_buffer for next loop
//                                 break;
//                             }
//                         }
//                     }
//                     live_ram_buffer.clear();
//                 }
//             }

//             // Yield dynamically
//             if (s_disk_fifo && !s_disk_fifo->empty() && is_online) {
//                 vTaskDelay(1); // 1ms high-speed catch-up tick
//             } else {
//                 vTaskDelay(pdMS_TO_TICKS(10)); // 10ms standard tick
//             }
//         }

//         vTaskDelete(nullptr);
//     }

// } // namespace Network


































// #include "Net manager.hpp"

// // Internal Network & Storage Dependencies
// #include "KcpClient.hpp"
// #include "WiFiManager.hpp"
// #include "FIFO_file.hpp"
// #include "encryption/interface.hpp"

// // System & FreeRTOS Headers
// #include <mutex>
// #include <queue>
// #include <atomic>
// #include <cstring>
// #include <memory>
// #include <vector>

// #include "esp_log.h"
// #include "esp_timer.h"
// #include "esp_random.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// namespace Network {

//     static const char* TAG = "NETWORK_MANAGER";

//     // =========================================================================
//     // Handshake State Engine
//     // =========================================================================
//     enum class HandshakeState {
//         IDLE,           // Offline or awaiting connection
//         WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
//         ESTABLISHED     // Handshake complete, AES key active for live traffic
//     };

//     // =========================================================================
//     // Static / Internal Handles
//     // =========================================================================
//     static std::unique_ptr<KcpClient>   s_kcp_client;
//     static std::unique_ptr<file::FIFO>  s_disk_fifo;
//     static uint32_t                     s_conv_id = 0;

//     // Crypto & Handshake session state
//     static std::vector<uint8_t>         s_aes_key;
//     static HandshakeState               s_hs_state = HandshakeState::IDLE;
//     static std::vector<uint8_t>         s_cached_handshake_pkt; 
//     static uint32_t                     s_last_handshake_attempt_ms = 0;
//     static uint32_t                     s_handshake_retry_count = 0;

//     // Worker Task Management
//     static TaskHandle_t                 s_worker_task_handle = nullptr;
//     static std::atomic<bool>            s_worker_running{false};

//     // Ping-Pong Double-Buffering (Outbound)
//     static std::mutex                               s_tx_mutex;
//     static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Appended by producer threads
//     static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Processed exclusively by worker task

//     // Inbound Rx Queue (Thread-safe consumer access)
//     static std::mutex                               s_rx_mutex;
//     static std::queue<std::vector<uint8_t>>         s_rx_queue;

//     // Tuning Parameters
//     static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;       // Initial retry backoff
//     static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000;      // Upper backoff ceiling
//     static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;       // Producer drop threshold (RAM backpressure)
//     static constexpr size_t   DRAIN_BURST_PER_TICK    = 40;        // Max packets drained per 1ms cycle
//     static constexpr size_t   FIFO_CACHE_CAPACITY     = 128 * 1024;// 128 KB RAM buffer inside FIFO

//     // Forward declaration of internal task & helpers
//     static void network_worker_task(void* pvParameters);
//     static void handle_autonomous_handshake(uint32_t now_ms);

//     // =========================================================================
//     // Public API Implementation
//     // =========================================================================

//     void init(const char* host, const char* port) {
//         s_conv_id = esp_random();

//         // 1. Initialize FIFO with a 128 KB RAM cache (holds ~35s of Opus before touching Flash!)
//         s_disk_fifo = std::make_unique<file::FIFO>("packageStoredFile", FIFO_CACHE_CAPACITY);

//         // 2. Initialize low-level KCP Client
//         s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
//         s_kcp_client->initialize(host, port);

//         // 3. Reset internal handshake and session state
//         s_aes_key.clear();
//         s_cached_handshake_pkt.clear();
//         s_hs_state = HandshakeState::IDLE;
//         s_handshake_retry_count = 0;

//         // 4. Spawn dedicated network worker task (Pinned to Core 1 for optimal I/O throughput)
//         if (!s_worker_running.load()) {
//             s_worker_running.store(true);
//             xTaskCreatePinnedToCore(
//                 network_worker_task,
//                 "net_worker_io",
//                 8192,                   // Stack size (8 KB)
//                 nullptr,
//                 configMAX_PRIORITIES - 3,// Priority
//                 &s_worker_task_handle,
//                 1                       // Core 1
//             );
//         }

//         ESP_LOGI(TAG, "Network manager initialized (conv_id: %u, FIFO RAM cache: %u KB)", 
//                  s_conv_id, (unsigned)(FIFO_CACHE_CAPACITY / 1024));
//     }

//     void close_connection() {
//         s_worker_running.store(false);
//         s_worker_task_handle = nullptr;

//         if (s_disk_fifo) {
//             s_disk_fifo->flush(); // Commit any unwritten RAM cache to flash on shutdown
//         }

//         if (s_kcp_client) {
//             s_kcp_client->close_connection();
//         }

//         s_hs_state = HandshakeState::IDLE;
//         ESP_LOGI(TAG, "Network connection closed cleanly");
//     }

//     int send(const std::vector<uint8_t>& data) {
//         if (data.empty()) {
//             return 0;
//         }

//         std::lock_guard<std::mutex> lock(s_tx_mutex);
//         if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
//             ESP_LOGW(TAG, "Outgoing RAM queue full; message rejected (Backpressure)");
//             return -1;
//         }

//         // Store raw unencrypted bytes.
//         // Worker task encrypts on-the-fly at the exact moment of transmission.
//         s_incoming_queue.push_back(data);
//         return 0;
//     }

//     std::vector<uint8_t> poll_and_receive() {
//         std::lock_guard<std::mutex> lock(s_rx_mutex);
//         if (s_rx_queue.empty()) {
//             return {};
//         }

//         std::vector<uint8_t> data = std::move(s_rx_queue.front());
//         s_rx_queue.pop();
//         return data;
//     }

//     // Retained for API compatibility; ticking is managed automatically inside network_worker_task
//     void update(uint32_t /*current_ms*/) {
//         // No-op
//     }

//     // =========================================================================
//     // Internal Autonomous Handshake Engine (Strictly Worker Thread Only)
//     // =========================================================================

//     static void handle_autonomous_handshake(uint32_t now_ms) {
//         bool wifi_ok = wifi::isConnected();

//         // If Wi-Fi drops, reset the handshake state machine cleanly
//         if (!wifi_ok) {
//             if (s_hs_state != HandshakeState::IDLE) {
//                 ESP_LOGW(TAG, "Wi-Fi link lost: Handshake state reset to IDLE");
//                 s_hs_state = HandshakeState::IDLE;
//                 s_cached_handshake_pkt.clear();
//                 s_handshake_retry_count = 0;
//             }
//             return;
//         }

//         switch (s_hs_state) {
//             case HandshakeState::IDLE: {
//                 ESP_LOGI(TAG, "Wi-Fi ready: Generating ECDH local keypair...");
//                 auto my_key_opt = encryption::ecdh_step1_generate_my_key();
//                 if (!my_key_opt.has_value()) {
//                     ESP_LOGE(TAG, "Key generation failed; will retry next tick");
//                     return;
//                 }

//                 const auto& pub_key_bytes = my_key_opt.value();
//                 s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());

//                 // Packet Format: [conv_id (4B)] + [Client Public Key]
//                 std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
//                 std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

//                 // First raw UDP attempt
//                 s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
//                 s_last_handshake_attempt_ms = now_ms;
//                 s_handshake_retry_count = 1;
//                 s_hs_state = HandshakeState::WAIT_ACK;

//                 ESP_LOGI(TAG, "Handshake dispatched (conv_id: %u, attempt #1)", s_conv_id);
//                 break;
//             }

//             case HandshakeState::WAIT_ACK: {
//                 // Exponential backoff capped at MAX_HANDSHAKE_RETRY_MS
//                 uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
//                 if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
//                     retry_interval = MAX_HANDSHAKE_RETRY_MS;
//                 }

//                 if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
//                     s_handshake_retry_count++;
//                     ESP_LOGW(TAG, "Handshake ACK timeout; retransmitting attempt #%u...", s_handshake_retry_count);

//                     // Retransmit the identical packet (Never regenerate keys on retry)
//                     s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
//                     s_last_handshake_attempt_ms = now_ms;
//                 }
//                 break;
//             }

//             case HandshakeState::ESTABLISHED:
//                 // Active session; nothing to do
//                 break;
//         }
//     }

//     // =========================================================================
//     // Single Consumer Task (KCP, Crypto, FIFO Storage)
//     // =========================================================================

//   static void network_worker_task(void* /*pvParameters*/) {
//         ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

//         // --- Performance Profiling Counters (Aggregated & logged every 1 second) ---
//         int64_t  perf_window_start_us   = esp_timer_get_time();
//         uint32_t stat_loop_count        = 0;
//         int64_t  stat_total_loop_us     = 0;

//         int64_t  stat_kcp_update_us     = 0;
//         int64_t  stat_kcp_poll_recv_us  = 0;
//         int64_t  stat_queue_swap_us     = 0;
//         int64_t  stat_fifo_push_us      = 0;
//         int64_t  stat_fifo_pop_us       = 0;
//         int64_t  stat_crypto_encrypt_us = 0;
//         int64_t  stat_kcp_send_us       = 0;
//         int64_t  stat_fifo_flush_us     = 0;

//         uint32_t stat_fifo_popped_pkts  = 0;
//         uint32_t stat_live_sent_pkts    = 0;

//         while (s_worker_running.load()) {
//             int64_t loop_start_us = esp_timer_get_time();
//             uint32_t now_ms = static_cast<uint32_t>(loop_start_us / 1000ULL);

//             // 1. Maintain Autonomous Handshake
//             handle_autonomous_handshake(now_ms);

//             // 2. KCP Clock Update
//             if (s_kcp_client) {
//                 int64_t t0 = esp_timer_get_time();
//                 s_kcp_client->update(now_ms);
//                 stat_kcp_update_us += (esp_timer_get_time() - t0);
//             }

//             // 3. Receive Packets & Handle Handshake Response
//             if (s_kcp_client) {
//                 int64_t t0 = esp_timer_get_time();
//                 auto incoming_data = s_kcp_client->poll_and_receive();
                
//                 if (!incoming_data.empty()) {
//                     if (s_hs_state == HandshakeState::WAIT_ACK) {
//                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
//                         if (shared_secret.has_value()) {
//                             s_aes_key = shared_secret.value();
//                             s_hs_state = HandshakeState::ESTABLISHED;
//                             s_cached_handshake_pkt.clear();
//                             ESP_LOGI(TAG, "Handshake completed! Session AES-GCM key established.");
//                         } else {
//                             ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
//                         }
//                     } else if (s_hs_state == HandshakeState::ESTABLISHED) {
//                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
//                         if (decrypted.has_value()) {
//                             std::lock_guard<std::mutex> lock(s_rx_mutex);
//                             s_rx_queue.push(std::move(decrypted.value()));
//                         } else {
//                             ESP_LOGE(TAG, "Failed to decrypt incoming packet");
//                         }
//                     }
//                 }
//                 stat_kcp_poll_recv_us += (esp_timer_get_time() - t0);
//             }

//             // 4. Ping-Pong Double-Buffering Swap
//             {
//                 int64_t t0 = esp_timer_get_time();
//                 std::lock_guard<std::mutex> lock(s_tx_mutex);
//                 if (!s_incoming_queue.empty()) {
//                     s_sending_queue.swap(s_incoming_queue);
//                 }
//                 stat_queue_swap_us += (esp_timer_get_time() - t0);
//             }

//             // 5. Unified Zero-Thrashing Transmission Pipeline
//             bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);
//             bool is_draining_fifo = false;

//             if (!is_online) {
//                 // OFFLINE PATH: Wi-Fi down or handshaking -> buffer into FIFO
//                 if (!s_sending_queue.empty() && s_disk_fifo) {
//                     int64_t t0 = esp_timer_get_time();
//                     for (auto& packet : s_sending_queue) {
//                         s_disk_fifo->push_back(std::move(packet));
//                     }
//                     stat_fifo_push_us += (esp_timer_get_time() - t0);
//                     s_sending_queue.clear();
//                 }
//             } else {
//                 // ONLINE PATH: Strict In-Order Delivery
//                 if (s_disk_fifo && !s_disk_fifo->empty()) {
//                     is_draining_fifo = true;

//                     // Append fresh live packets to tail of FIFO to preserve chronology
//                     if (!s_sending_queue.empty()) {
//                         int64_t t0 = esp_timer_get_time();
//                         for (auto& packet : s_sending_queue) {
//                             s_disk_fifo->push_back(std::move(packet));
//                         }
//                         stat_fifo_push_us += (esp_timer_get_time() - t0);
//                         s_sending_queue.clear();
//                     }

//                     // High-speed drain
//                     size_t popped_count = 0;
//                     while (!s_disk_fifo->empty() && popped_count < DRAIN_BURST_PER_TICK) {
//                         // Measure Pop time
//                         int64_t t_pop = esp_timer_get_time();
//                         auto raw_pkt = s_disk_fifo->Pop();
//                         stat_fifo_pop_us += (esp_timer_get_time() - t_pop);

//                         if (raw_pkt.empty()) {
//                             break;
//                         }

//                         // Measure AES-GCM Encrypt time
//                         int64_t t_crypto = esp_timer_get_time();
//                         auto encrypted = encryption::aes_gcm_encrypt(raw_pkt, s_aes_key);
//                         stat_crypto_encrypt_us += (esp_timer_get_time() - t_crypto);

//                         // Measure KCP Send time
//                         if (encrypted.has_value()) {
//                             int64_t t_send = esp_timer_get_time();
//                             int res = s_kcp_client->send(encrypted->data(), encrypted->size());
//                             stat_kcp_send_us += (esp_timer_get_time() - t_send);

//                             if (res == -1) {
//                                 // Socket / flight buffer full
//                                 break;
//                             }
//                         } else {
//                             ESP_LOGE(TAG, "Failed to encrypt packet during drain");
//                         }
//                         popped_count++;
//                         stat_fifo_popped_pkts++;
//                     }

//                     // Persist pointer and truncate if complete
//                     if (s_disk_fifo->empty()) {
//                         int64_t t_flush = esp_timer_get_time();
//                         s_disk_fifo->flush();
//                         stat_fifo_flush_us += (esp_timer_get_time() - t_flush);
//                         ESP_LOGI(TAG, ">>> FIFO backlog completely caught up & drained! <<<");
//                     }
//                 } 
//                 // CASE B: FIFO empty -> Send directly from live RAM queue
//                 else if (!s_sending_queue.empty()) {
//                     for (const auto& raw_packet : s_sending_queue) {
//                         int64_t t_crypto = esp_timer_get_time();
//                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
//                         stat_crypto_encrypt_us += (esp_timer_get_time() - t_crypto);

//                         if (encrypted.has_value()) {
//                             int64_t t_send = esp_timer_get_time();
//                             int res = s_kcp_client->send(encrypted->data(), encrypted->size());
//                             stat_kcp_send_us += (esp_timer_get_time() - t_send);

//                             if (res == -1) {
//                                 // Fallback into FIFO if socket saturated
//                                 int64_t t_push = esp_timer_get_time();
//                                 s_disk_fifo->push_back(raw_packet);
//                                 stat_fifo_push_us += (esp_timer_get_time() - t_push);
//                             } else {
//                                 stat_live_sent_pkts++;
//                             }
//                         } else {
//                             ESP_LOGE(TAG, "Failed to encrypt live packet");
//                         }
//                     }
//                     s_sending_queue.clear();
//                 }
//             }

//             int64_t loop_end_us = esp_timer_get_time();
//             stat_total_loop_us += (loop_end_us - loop_start_us);
//             stat_loop_count++;

//             // =================================================================
//             // LOG STATS ONCE EVERY 1,000 ms (1 SECOND)
//             // =================================================================
//             if (loop_end_us - perf_window_start_us >= 1000000ULL) {
//                 // Only print if there was network/FIFO activity to prevent log spam
//                 if (stat_fifo_popped_pkts > 0 || stat_live_sent_pkts > 0 || stat_fifo_push_us > 0) {
//                     ESP_LOGW(TAG, "--- PERF (1s): Loops: %u | FIFO Popped: %u | Live Sent: %u ---",
//                              stat_loop_count, stat_fifo_popped_pkts, stat_live_sent_pkts);
//                     ESP_LOGW(TAG, "  FIFO Pop:    %7lld us  (avg: %5.1f us/pkt)", 
//                              stat_fifo_pop_us, stat_fifo_popped_pkts ? (double)stat_fifo_pop_us / stat_fifo_popped_pkts : 0.0);
//                     ESP_LOGW(TAG, "  FIFO Push:   %7lld us", stat_fifo_push_us);
//                     ESP_LOGW(TAG, "  FIFO Flush:  %7lld us", stat_fifo_flush_us);
//                     ESP_LOGW(TAG, "  Crypto Enc:  %7lld us", stat_crypto_encrypt_us);
//                     ESP_LOGW(TAG, "  KCP Send:    %7lld us", stat_kcp_send_us);
//                     ESP_LOGW(TAG, "  KCP Update:  %7lld us", stat_kcp_update_us);
//                     ESP_LOGW(TAG, "  KCP PollRx:  %7lld us", stat_kcp_poll_recv_us);
//                     ESP_LOGW(TAG, "  Queue Swap:  %7lld us", stat_queue_swap_us);
//                     ESP_LOGW(TAG, "  Total Loop:  %7lld us  (CPU Duty: %.1f%%)", 
//                              stat_total_loop_us, (stat_total_loop_us / 1000000.0) * 100.0);
//                 }

//                 // Reset counters for the next 1-second window
//                 stat_loop_count         = 0;
//                 stat_total_loop_us      = 0;
//                 stat_kcp_update_us      = 0;
//                 stat_kcp_poll_recv_us   = 0;
//                 stat_queue_swap_us      = 0;
//                 stat_fifo_push_us       = 0;
//                 stat_fifo_pop_us        = 0;
//                 stat_crypto_encrypt_us  = 0;
//                 stat_kcp_send_us        = 0;
//                 stat_fifo_flush_us      = 0;
//                 stat_fifo_popped_pkts   = 0;
//                 stat_live_sent_pkts     = 0;
//                 perf_window_start_us    = esp_timer_get_time();
//             }

//             // 6. Dynamic Cadence
//             if (is_draining_fifo && s_disk_fifo && !s_disk_fifo->empty()) {
//                 vTaskDelay(1); // 1ms high-speed catch-up tick
//             } else {
//                 vTaskDelay(pdMS_TO_TICKS(10)); // 10ms standard tick
//             }
//         }

//         vTaskDelete(nullptr);
//     }

// } // namespace Network














































#include "Net manager.hpp"

// Internal Network Dependencies
#include "KcpClient.hpp"
#include "WiFiManager.hpp"
#include "encryption/interface.hpp"

// System & FreeRTOS Headers
#include <mutex>
#include <queue>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Network {

    static const char* TAG = "NETWORK_MANAGER";

    // =========================================================================
    // Handshake State Engine
    // =========================================================================
    enum class HandshakeState {
        IDLE,           // Offline or awaiting connection
        WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
        ESTABLISHED     // Handshake complete, AES key active for live traffic
    };

    // =========================================================================
    // Static / Internal Handles
    // =========================================================================
    static std::unique_ptr<KcpClient>   s_kcp_client;
    static uint32_t                     s_conv_id = 0;

    // Crypto & Handshake session state
    static std::vector<uint8_t>         s_aes_key;
    static HandshakeState               s_hs_state = HandshakeState::IDLE;
    static std::vector<uint8_t>         s_cached_handshake_pkt; 
    static uint32_t                     s_last_handshake_attempt_ms = 0;
    static uint32_t                     s_handshake_retry_count = 0;

    // Worker Task Management
    static TaskHandle_t                 s_worker_task_handle = nullptr;
    static std::atomic<bool>            s_worker_running{false};

    // Double-Buffering (Outbound)
    static std::mutex                               s_tx_mutex;
    static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Appended by producer threads
    static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Consumed by worker task

    // Inbound Rx Queue (Thread-safe consumer access)
    static std::mutex                               s_rx_mutex;
    static std::queue<std::vector<uint8_t>>         s_rx_queue;

    // Tuning Parameters
    static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Initial retry backoff
    static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Upper backoff ceiling
    static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;  // Producer drop threshold (RAM backpressure)
    static constexpr TickType_t WORKER_TICK_DELAY     = pdMS_TO_TICKS(10); // Standard 10ms KCP tick

    // Forward declaration of internal task & helpers
    static void network_worker_task(void* pvParameters);
    static void handle_autonomous_handshake(uint32_t now_ms);

    // =========================================================================
    // Public API Implementation
    // =========================================================================

    void init(const char* host, const char* port) {
        s_conv_id = esp_random();

        // 1. Initialize low-level KCP Client
        s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
        s_kcp_client->initialize(host, port);

        // 2. Reset session & handshake state
        s_aes_key.clear();
        s_cached_handshake_pkt.clear();
        s_hs_state = HandshakeState::IDLE;
        s_handshake_retry_count = 0;

        // 3. Clear queues
        {
            std::lock_guard<std::mutex> lock_tx(s_tx_mutex);
            s_incoming_queue.clear();
            s_sending_queue.clear();
        }
        {
            std::lock_guard<std::mutex> lock_rx(s_rx_mutex);
            std::queue<std::vector<uint8_t>> empty_q;
            std::swap(s_rx_queue, empty_q);
        }

        // 4. Spawn dedicated worker task (Pinned to Core 1)
        if (!s_worker_running.load()) {
            s_worker_running.store(true);
            xTaskCreatePinnedToCore(
                network_worker_task,
                "net_worker_io",
                8192,
                nullptr,
                configMAX_PRIORITIES - 3,
                &s_worker_task_handle,
                1
            );
        }

        ESP_LOGI(TAG, "Network manager initialized (conv_id: %u)", s_conv_id);
    }

    void close_connection() {
        s_worker_running.store(false);

        if (s_kcp_client) {
            s_kcp_client->close_connection();
        }

        s_hs_state = HandshakeState::IDLE;
        ESP_LOGI(TAG, "Network connection closed");
    }

    // int send(const std::vector<uint8_t>& data) {
    //     if (data.empty()) {
    //         return 0;
    //     }

    //     std::lock_guard<std::mutex> lock(s_tx_mutex);
    //     if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
    //         ESP_LOGW(TAG, "Outgoing RAM queue full; message rejected (Backpressure)");
    //         return -1;
    //     }

    //     s_incoming_queue.push_back(data);
    //     return 0;
    // }
int send(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(s_tx_mutex);

    // --- Performance Tracking State ---
    static int64_t  s_last_report_us = 0;
    static uint32_t s_window_sent    = 0;
    static uint32_t s_window_dropped = 0;
    static size_t   s_window_bytes   = 0;

    int64_t now_us = esp_timer_get_time();
    if (s_last_report_us == 0) {
        s_last_report_us = now_us;
    }

    // 1. Backpressure Check
    if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
        s_window_dropped++;
        ESP_LOGW(TAG, "Outgoing RAM queue full; message rejected (Backpressure)");
        return -1;
    }

    // 2. Queue the Packet
    s_incoming_queue.push_back(data);
    s_window_sent++;
    s_window_bytes += data.size();

    // 3. Periodic 1-Second Send Frequency Report
    int64_t elapsed_us = now_us - s_last_report_us;
    if (elapsed_us >= 1000000ULL) {
        float elapsed_sec = elapsed_us / 1000000.0f;
        float pps = s_window_sent / elapsed_sec;
        float kb_per_sec = (s_window_bytes / 1024.0f) / elapsed_sec;

        ESP_LOGI(TAG, "[TX STATS 1s] Rate: %.1f pkts/s | Bandwidth: %.2f KB/s | Dropped: %u | Queue Depth: %u",
                 pps,
                 kb_per_sec,
                 s_window_dropped,
                 static_cast<unsigned>(s_incoming_queue.size()));

        // Reset counters for the next 1-second window
        s_window_sent    = 0;
        s_window_dropped = 0;
        s_window_bytes   = 0;
        s_last_report_us = now_us;
    }

    return 0;
}
    std::vector<uint8_t> poll_and_receive() {
        std::lock_guard<std::mutex> lock(s_rx_mutex);
        if (s_rx_queue.empty()) {
            return {};
        }

        std::vector<uint8_t> data = std::move(s_rx_queue.front());
        s_rx_queue.pop();
        return data;
    }

    void update(uint32_t /*current_ms*/) {
        // No-op: Ticking is managed internally inside network_worker_task
    }

    // =========================================================================
    // Internal Autonomous Handshake Engine
    // =========================================================================

    static void handle_autonomous_handshake(uint32_t now_ms) {
        if (!wifi::isConnected()) {
            if (s_hs_state != HandshakeState::IDLE) {
                ESP_LOGW(TAG, "Wi-Fi link lost: Resetting handshake state to IDLE");
                s_hs_state = HandshakeState::IDLE;
                s_cached_handshake_pkt.clear();
                s_handshake_retry_count = 0;
            }
            return;
        }

        switch (s_hs_state) {
            case HandshakeState::IDLE: {
                ESP_LOGI(TAG, "Wi-Fi ready: Generating ECDH local keypair...");
                auto my_key_opt = encryption::ecdh_step1_generate_my_key();
                if (!my_key_opt.has_value()) {
                    ESP_LOGE(TAG, "Key generation failed; will retry next tick");
                    return;
                }

                const auto& pub_key_bytes = my_key_opt.value();
                s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());

                // Packet Format: [conv_id (4B)] + [Client Public Key]
                std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
                std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

                s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
                s_last_handshake_attempt_ms = now_ms;
                s_handshake_retry_count = 1;
                s_hs_state = HandshakeState::WAIT_ACK;

                ESP_LOGI(TAG, "Handshake dispatched (conv_id: %u, attempt #1)", s_conv_id);
                break;
            }

            case HandshakeState::WAIT_ACK: {
                uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
                if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
                    retry_interval = MAX_HANDSHAKE_RETRY_MS;
                }

                if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
                    s_handshake_retry_count++;
                    ESP_LOGW(TAG, "Handshake ACK timeout; retransmitting attempt #%u...", s_handshake_retry_count);
                    s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
                    s_last_handshake_attempt_ms = now_ms;
                }
                break;
            }

            case HandshakeState::ESTABLISHED:
                break;
        }
    }

    // =========================================================================
    // Network Worker Task (KCP Clock, Crypto, In/Out Streaming)
    // =========================================================================

    // static void network_worker_task(void* /*pvParameters*/) {
    //     ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

    //     while (s_worker_running.load()) {
    //         uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

    //         // 1. Maintain Handshake State Machine
    //         handle_autonomous_handshake(now_ms);

    //         // 2. KCP Clock Update
    //         if (s_kcp_client) {
    //             s_kcp_client->update(now_ms);
    //         }

    //         // 3. Receive Packets & Process Crypto Responses
    //         if (s_kcp_client) {
    //             auto incoming_data = s_kcp_client->poll_and_receive();

    //             if (!incoming_data.empty()) {
    //                 if (s_hs_state == HandshakeState::WAIT_ACK) {
    //                     auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
    //                     if (shared_secret.has_value()) {
    //                         s_aes_key = shared_secret.value();
    //                         s_hs_state = HandshakeState::ESTABLISHED;
    //                         s_cached_handshake_pkt.clear();
    //                         ESP_LOGI(TAG, "Handshake completed! Session AES-GCM key established.");
    //                     } else {
    //                         ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
    //                     }
    //                 } else if (s_hs_state == HandshakeState::ESTABLISHED) {
    //                     auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
    //                     if (decrypted.has_value()) {
    //                         std::lock_guard<std::mutex> lock(s_rx_mutex);
    //                         s_rx_queue.push(std::move(decrypted.value()));
    //                     } else {
    //                         ESP_LOGE(TAG, "Failed to decrypt incoming packet");
    //                     }
    //                 }
    //             }
    //         }

    //         // 4. Swap Producer Buffer into Worker Buffer
    //         {
    //             std::lock_guard<std::mutex> lock(s_tx_mutex);
    //             if (!s_incoming_queue.empty()) {
    //                 s_sending_queue.swap(s_incoming_queue);
    //             }
    //         }

    //         // 5. Encrypt & Transmit
    //         bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

    //         if (!s_sending_queue.empty()) {
    //             if (is_online && s_kcp_client) {
    //                 for (const auto& raw_packet : s_sending_queue) {
    //                     auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
    //                     if (encrypted.has_value()) {
    //                         int res = s_kcp_client->send(encrypted->data(), encrypted->size());
    //                         if (res == -1) {
    //                             ESP_LOGW(TAG, "KCP send buffer saturated, dropping packet");
    //                         }
    //                     } else {
    //                         ESP_LOGE(TAG, "Failed to encrypt packet");
    //                     }
    //                 }
    //             } else {
    //                 ESP_LOGW(TAG, "Dropped %u packet(s): Connection not established", 
    //                          static_cast<unsigned>(s_sending_queue.size()));
    //             }
    //             s_sending_queue.clear();
    //         }

    //         // 6. Loop Interval
    //         vTaskDelay(WORKER_TICK_DELAY);
    //     }

    //     s_worker_task_handle = nullptr;
    //     vTaskDelete(nullptr);
    // }
static void network_worker_task(void* /*pvParameters*/) {
        ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

        // --- Performance Profiling Timers (Aggregated & logged every 1s) ---
        int64_t  perf_window_start_us = esp_timer_get_time();
        uint32_t stat_loop_count      = 0;
        uint32_t stat_sent_pkts       = 0;
        int64_t  stat_work_time_us    = 0;
        int64_t  stat_crypto_time_us  = 0;
        int64_t  stat_kcp_time_us     = 0;

        // Tracks task wake time for drift-free 10ms intervals
        TickType_t last_wake_time = xTaskGetTickCount();

        while (s_worker_running.load()) {
            int64_t loop_start_us = esp_timer_get_time();
            uint32_t now_ms = static_cast<uint32_t>(loop_start_us / 1000ULL);

            // 1. Maintain Handshake State Machine
            handle_autonomous_handshake(now_ms);

            // 2. KCP Clock Update & Inbound Processing
            int64_t t_kcp_start = esp_timer_get_time();
            if (s_kcp_client) {
                s_kcp_client->update(now_ms);

                // 3. Receive Packets & Process Crypto Responses
                auto incoming_data = s_kcp_client->poll_and_receive();

                if (!incoming_data.empty()) {
                    if (s_hs_state == HandshakeState::WAIT_ACK) {
                        auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
                        if (shared_secret.has_value()) {
                            s_aes_key = shared_secret.value();
                            s_hs_state = HandshakeState::ESTABLISHED;
                            s_cached_handshake_pkt.clear();
                            ESP_LOGI(TAG, "Handshake completed! Session AES-GCM key established.");
                        } else {
                            ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
                        }
                    } else if (s_hs_state == HandshakeState::ESTABLISHED) {
                        int64_t t_crypto = esp_timer_get_time();
                        auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
                        stat_crypto_time_us += (esp_timer_get_time() - t_crypto);

                        if (decrypted.has_value()) {
                            std::lock_guard<std::mutex> lock(s_rx_mutex);
                            s_rx_queue.push(std::move(decrypted.value()));
                        } else {
                            ESP_LOGE(TAG, "Failed to decrypt incoming packet");
                        }
                    }
                }
            }
            stat_kcp_time_us += (esp_timer_get_time() - t_kcp_start);

            // 4. Swap Producer Buffer into Worker Buffer
            {
                std::lock_guard<std::mutex> lock(s_tx_mutex);
                if (!s_incoming_queue.empty()) {
                    s_sending_queue.swap(s_incoming_queue);
                }
            }

            // 5. Encrypt & Transmit
            bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

            if (!s_sending_queue.empty()) {
                if (is_online && s_kcp_client) {
                    for (const auto& raw_packet : s_sending_queue) {
                        // Measure AES encryption duration
                        int64_t t_crypto = esp_timer_get_time();
                        auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
                        stat_crypto_time_us += (esp_timer_get_time() - t_crypto);

                        if (encrypted.has_value()) {
                            // Measure KCP send duration
                            int64_t t_send = esp_timer_get_time();
                            int res = s_kcp_client->send(encrypted->data(), encrypted->size());
                            stat_kcp_time_us += (esp_timer_get_time() - t_send);

                            if (res == -1) {
                                ESP_LOGW(TAG, "KCP send buffer saturated, dropping packet");
                            } else {
                                stat_sent_pkts++;
                            }
                        } else {
                            ESP_LOGE(TAG, "Failed to encrypt packet");
                        }
                    }
                } else {
                    ESP_LOGW(TAG, "Dropped %u packet(s): Connection not established", 
                             static_cast<unsigned>(s_sending_queue.size()));
                }
                s_sending_queue.clear();
            }

            int64_t loop_end_us = esp_timer_get_time();
            stat_work_time_us += (loop_end_us - loop_start_us);
            stat_loop_count++;

            // =================================================================
            // 1-Second Periodic Performance Report
            // =================================================================
            if (loop_end_us - perf_window_start_us >= 1000000ULL) {
                // Log only when active to avoid flooding the console when idle
                if (stat_sent_pkts > 0 || stat_work_time_us > 10000ULL) {
                    double cpu_duty = (stat_work_time_us / 1000000.0) * 100.0;
                    ESP_LOGI(TAG, "[1s PERF] Loops: %u | Sent: %u | Active: %lld us (Duty: %.1f%%) | KCP: %lld us | Crypto: %lld us",
                             stat_loop_count, stat_sent_pkts, stat_work_time_us, cpu_duty, stat_kcp_time_us, stat_crypto_time_us);

                             // Add this inside the 1-second PERF block in network_worker_task:
size_t free_dma   = heap_caps_get_free_size(MALLOC_CAP_DMA);
size_t max_dma    = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
size_t total_free = esp_get_free_heap_size();

ESP_LOGI(TAG, "[RAM 1s] Total Free: %u B | DMA Free: %u B | Largest DMA Block: %u B",
         (unsigned)total_free, (unsigned)free_dma, (unsigned)max_dma);
                }

                // Reset statistics for the next 1-second window
                stat_loop_count      = 0;
                stat_sent_pkts       = 0;
                stat_work_time_us    = 0;
                stat_crypto_time_us  = 0;
                stat_kcp_time_us     = 0;
                perf_window_start_us = esp_timer_get_time();
            }

            // 6. Cadence: Fixed 10ms intervals without tick drift
            vTaskDelayUntil(&last_wake_time, WORKER_TICK_DELAY);
        }

        s_worker_task_handle = nullptr;
        vTaskDelete(nullptr);
    }
} // namespace Network