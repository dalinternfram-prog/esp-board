// // // #include "Net manager.hpp"
// // // #include <memory>
// // // #include <vector>
// // // #include "encryption/interface.hpp"

// // // namespace Network { 

// // //     // Internal state variables (formerly class members)
  
    
// // //     static std::unique_ptr<KcpClient> u_KcpClient;
// // //     static std::string fileName = "packageStoredFile";
// // //   //  static std::unique_ptr<file::fileVector> packageStoredFile = std::make_unique<file::fileVector>(fileName);
// // //     static uint32_t conv_id; 


// // // //TODO:this is error now,since we store un-encrypted data here;
// // //     // static void  try_send_disk_package(){
// // //     //     if(!wifi::isConnected())return;
// // //     //     if(!completeHandshake)return;
// // //     //             // has wifi; we check the unsend one;
// // //     //         const auto currentSize = packageStoredFile->GetCurrentSize();
// // //     //         for(int i=0;i<currentSize;++i){
// // //     //           auto unsend_ = packageStoredFile->Pop();
 
// // //     //                 auto Opt_encrypted_data = encryption::aes_gcm_encrypt(unsend_,aes_key);
// // //     //     if (!Opt_encrypted_data.has_value()) return  ;

// // //     //     auto encrypted_data = Opt_encrypted_data.value();

// // //     //                // we encrypt here;
// // //     //     int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
// // //     //     if (r == -1) {
// // //     //         ESP_LOGE(TAG, "send failed, reason: unknown");
// // //     //     }
// // //     //     return  ;

// // //     //         }
 
// // //     // }
// // //     // Initialization (replaces the constructor)
// // //     void init( const char* host, const char* port) {
// // //          conv_id = 0;
 
// // //         conv_id = esp_random();
 
// // //         // Initialize your KcpClient here, e.g.:
// // //          u_KcpClient = std::make_unique<KcpClient>(conv_id);
// // //         u_KcpClient->initialize(host,port);
   
// // //    //try_send_disk_package();

// // //     }

// // //     void close_connection() {
// // //         if (u_KcpClient) {
// // //             u_KcpClient->close_connection();
// // //         }
// // //     }

// // //     // Core Tick & Data Flow Methods
// // //     void update(uint32_t current_ms) {
// // //         if (u_KcpClient) {
// // //             u_KcpClient->update(current_ms);
// // //         }
 
// // //   // try_send_disk_package();


// // //     }
// // // // TODO: this should create a watch dog to watch the information from server;
// // //     std::vector<uint8_t> poll_and_receive() {
// // //         if (!u_KcpClient) return {};
        
        
// // //         auto encrypted_data = u_KcpClient->poll_and_receive();
// // //           if(encrypted_data.size()==0)return {};


// // //         if(!completeHandshake){
// // //             //* this is the server public key;
// // //             //* we extract it ;
// // //           std::optional<std::vector<uint8_t>> result =   encryption::ecdh_step2_compute_shared_secret(encrypted_data);
// // //           if(result.has_value()){
             
// // //             aes_key = result.value();
// // //             completeHandshake=true;

// // //             // --- ADD THIS CODE TO PRINT THE KEY AS HEX ---
// // //             std::string hex_str = "";
// // //             for (uint8_t b : aes_key) {
// // //                 char buf[3];
// // //                 snprintf(buf, sizeof(buf), "%02x", b);
// // //                 hex_str += buf;
// // //             }
// // //             ESP_LOGE(TAG, "Client Shared Secret (Hex): %s", hex_str.c_str());


// // //             // ---------------------------------------------
// // //             return {};
// // //           }
// // //         }
// // //         auto Opt_decrypted_data = encryption::aes_gcm_decrypt(encrypted_data);

// // //         if (!Opt_decrypted_data.has_value()) return {};
// // //         return Opt_decrypted_data.value();
// // //     }


    






// // // // TODO: when it's first send,we don't encrypt;
// // //     int send(const std::vector<uint8_t>& data) {

// // //         //TODO test;
// // // if(!wifi::isConnected()){
// // //      ESP_LOGI(TAG, "wifi disconnected ");
// // //             // store in disk;
// // //          //   packageStoredFile->push_back(data);
// // //             return -1;
// // // }
 
// // // //Todo test;
// // // if( !completeHandshake){
// // //            //ESP_LOGI(TAG, "  handshake has not finished yet");
// // //             // store in disk;
// // //           //  packageStoredFile->push_back(data);
// // //             return -1;
// // // }
 
  
// // //   // try_send_disk_package();

// // //          //  ESP_LOGW(TAG, " pay load size = %u bytes",data.size());
// // //         auto Opt_encrypted_data = encryption::aes_gcm_encrypt(data,aes_key);
// // //         if (!Opt_encrypted_data.has_value()) return -1;

// // //         auto encrypted_data = Opt_encrypted_data.value();

// // //                    // we encrypt here;

// // //                    //TODO: test;
// // //         int r = u_KcpClient->send(encrypted_data.data(), encrypted_data.size());
// // //         if (r == -1) {
// // //             ESP_LOGI(TAG, "send failed, reason: unknown");
// // //         }
// // //         return r;
// // //     }
// // //  bool send_handshakeToServer(){
// // // if (completeHandshake) {
// // //             ESP_LOGE(TAG, "duplicated handshake with server");
// // //             return false;
// // //         }

// // //         std::optional<std::vector<uint8_t>> generated_key = encryption::ecdh_step1_generate_my_key();
// // //         if (!generated_key.has_value()) {
// // //             ESP_LOGE(TAG, "!generated_key.has_value()");
// // //             return false;
// // //         }

// // //         const auto& pub_key_bytes = generated_key.value();
// // //         std::vector<uint8_t> handshake_packet;
// // //         handshake_packet.resize(sizeof(uint32_t) + pub_key_bytes.size());

// // //         // Copy conv_id into the first 4 bytes
// // //         std::memcpy(handshake_packet.data(), &conv_id, sizeof(uint32_t));

// // //         // Copy public key bytes right after it
// // //         std::memcpy(handshake_packet.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// // //         ESP_LOGW(TAG, "ready to send to server the handshake (conv_id: %u)", conv_id);

// // //         // FIX 2: Send the full handshake_packet instead of just the raw key
// // //         int r = u_KcpClient->raw_udp_send(handshake_packet.data(), handshake_packet.size());
// // //         if (r == -1) {
// // //             ESP_LOGE(TAG, "send failed, reason: unknown");
// // //             return false;
// // //         }
// // //         return true;
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

// //     // =========================================================================
// //     // Internal State & Handles
// //     // =========================================================================
// //     static std::unique_ptr<KcpClient>   u_KcpClient;
// //     static std::unique_ptr<file::FIFO>  packageStoredFile;
// //     static uint32_t                     conv_id = 0;

// //     // Background worker task management
// //     static TaskHandle_t                 s_worker_task_handle = nullptr;
// //     static std::atomic<bool>            s_worker_running{false};

// //     // --- Ping-Pong (Double Buffering) Queues for Outbound Data ---
// //     static std::mutex                               s_tx_mutex;
// //     static std::vector<std::vector<uint8_t>>        incoming_queue; // Appended by producers
// //     static std::vector<std::vector<uint8_t>>        sending_queue;  // Drained by network task

// //     // --- Thread-Safe Rx Queue for Inbound Data ---
// //     static std::mutex                               s_rx_mutex;
// //     static std::queue<std::vector<uint8_t>>         rx_queue;

// //     // Maximum RAM queue size before producers get rejected (backpressure safeguard)
// //     static constexpr size_t MAX_RAM_QUEUE_SIZE = 128;

// //     // Maximum disk packets to drain per loop tick (prevents choking KCP flow control)
// //     static constexpr size_t MAX_DISK_BURST_PER_TICK = 5;

// //     // Forward declaration of internal worker task
// //     static void network_worker_task(void* pvParameters);

// //     // =========================================================================
// //     // Lifecycle Management
// //     // =========================================================================
// //     void init(const char* host, const char* port) {
// //         conv_id = esp_random();

// //         // 1. Initialize persistent storage FIFO
// //         packageStoredFile = std::make_unique<file::FIFO>("packageStoredFile");

// //         // 2. Initialize low-level KCP Client
// //         u_KcpClient = std::make_unique<KcpClient>(conv_id);
// //         u_KcpClient->initialize(host, port);

// //         completeHandshake = false;
// //         aes_key.clear();

// //         // 3. Spawn dedicated network worker task (Core 1 recommended on ESP32)
// //         if (!s_worker_running.load()) {
// //             s_worker_running.store(true);
// //             xTaskCreatePinnedToCore(
// //                 network_worker_task,
// //                 "net_worker_io",
// //                 8192,                   // Stack size (8KB for TLS/Crypto/IO)
// //                 nullptr,
// //                 configMAX_PRIORITIES - 3,// High priority for latency-sensitive network I/O
// //                 &s_worker_task_handle,
// //                 1                       // Core 1 (separates network from Core 0 Wi-Fi stack)
// //             );
// //         }

// //         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", conv_id);
// //     }

// //     void close_connection() {
// //         s_worker_running.store(false);

// //         if (s_worker_task_handle != nullptr) {
// //             // Task will gracefully exit upon checking s_worker_running
// //             s_worker_task_handle = nullptr;
// //         }

// //         if (u_KcpClient) {
// //             u_KcpClient->close_connection();
// //         }

// //         ESP_LOGI(TAG, "Network connection closed");
// //     }

// //     // =========================================================================
// //     // Producer Interface (Thread-Safe, Called by ANY thread)
// //     // =========================================================================
// //     int send(const std::vector<uint8_t>& data) {
// //         if (data.empty()) {
// //             return 0;
// //         }

// //         std::lock_guard<std::mutex> lock(s_tx_mutex);

// //         // Protect RAM against out-of-memory if the network is disconnected for too long
// //         if (incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
// //             ESP_LOGW(TAG, "RAM queue full, packet dropped");
// //             return -1;
// //         }

// //         // Store RAW UNENCRYPTED data.
// //         // Encryption will take place right before transmission inside the worker task.
// //         incoming_queue.push_back(data);
// //         return 0;
// //     }

// //     std::vector<uint8_t> poll_and_receive() {
// //         std::lock_guard<std::mutex> lock(s_rx_mutex);
// //         if (rx_queue.empty()) {
// //             return {};
// //         }

// //         std::vector<uint8_t> data = std::move(rx_queue.front());
// //         rx_queue.pop();
// //         return data;
// //     }

// //     // Retained for API compatibility. The actual KCP tick runs inside network_worker_task.
// //     void update(uint32_t /*current_ms*/) {
// //         // No-op: handled concurrently in network_worker_task to prevent KCP race conditions
// //     }

// //     bool send_handshakeToServer() {
// //         if (completeHandshake) {
// //             ESP_LOGE(TAG, "Duplicated handshake request rejected");
// //             return false;
// //         }

// //         auto generated_key = encryption::ecdh_step1_generate_my_key();
// //         if (!generated_key.has_value()) {
// //             ESP_LOGE(TAG, "Failed to generate ECDH client keypair");
// //             return false;
// //         }

// //         const auto& pub_key_bytes = generated_key.value();
// //         std::vector<uint8_t> handshake_packet(sizeof(uint32_t) + pub_key_bytes.size());

// //         // Header: 4 bytes conv_id + Client Public Key
// //         std::memcpy(handshake_packet.data(), &conv_id, sizeof(uint32_t));
// //         std::memcpy(handshake_packet.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

// //         ESP_LOGI(TAG, "Dispatching raw UDP handshake (conv_id: %u)", conv_id);
// //         int r = u_KcpClient->raw_udp_send(handshake_packet.data(), handshake_packet.size());
// //         return (r != -1);
// //     }

// //     // =========================================================================
// //     // Consumer Worker Task (Single Thread: handles KCP, Flash, and Crypto)
// //     // =========================================================================
// //     static void network_worker_task(void* /*pvParameters*/) {
// //         ESP_LOGI(TAG, "Network worker task started on Core %d", xPortGetCoreID());

// //         while (s_worker_running.load()) {
// //             uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

// //             // -----------------------------------------------------------------
// //             // 1. KCP Clock Update (MUST run in same thread as kcp_send/recv)
// //             // -----------------------------------------------------------------
// //             if (u_KcpClient) {
// //                 u_KcpClient->update(now_ms);
// //             }

// //             // -----------------------------------------------------------------
// //             // 2. Poll Inbound Data & Process Handshake
// //             // -----------------------------------------------------------------
// //             if (u_KcpClient) {
// //                 auto incoming_data = u_KcpClient->poll_and_receive();
// //                 if (!incoming_data.empty()) {
// //                     if (!completeHandshake) {
// //                         // Inbound packet during handshake phase contains Server's Public Key
// //                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
// //                         if (shared_secret.has_value()) {
// //                             aes_key = shared_secret.value();
// //                             completeHandshake = true;
// //                             ESP_LOGI(TAG, "ECDH Shared Secret established successfully!");
// //                         } else {
// //                             ESP_LOGE(TAG, "Failed to compute shared secret from server packet");
// //                         }
// //                     } else {
// //                         // Handshake is established: decrypt application payload
// //                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
// //                         if (decrypted.has_value()) {
// //                             std::lock_guard<std::mutex> lock(s_rx_mutex);
// //                             rx_queue.push(std::move(decrypted.value()));
// //                         } else {
// //                             ESP_LOGE(TAG, "Failed to decrypt incoming packet");
// //                         }
// //                     }
// //                 }
// //             }

// //             // -----------------------------------------------------------------
// //             // 3. Ping-Pong Swap (O(1) under brief lock)
// //             // -----------------------------------------------------------------
// //             {
// //                 std::lock_guard<std::mutex> lock(s_tx_mutex);
// //                 if (!incoming_queue.empty()) {
// //                     sending_queue.swap(incoming_queue);
// //                 }
// //             }

// //             // -----------------------------------------------------------------
// //             // 4. State Decision: Connected vs Disconnected
// //             // -----------------------------------------------------------------
// //             bool is_online = wifi::isConnected() && completeHandshake;

// //             if (!is_online) {
// //                 // -------------------------------------------------------------
// //                 // OFFLINE PATH: Commit pending RAM batch to persistent FIFO
// //                 // -------------------------------------------------------------
// //                 if (!sending_queue.empty() && packageStoredFile) {
// //                     packageStoredFile->begin_push_mode();
// //                     for (auto& packet : sending_queue) {
// //                         // Persist plain data! Key is ephemeral and will change across boots.
// //                         packageStoredFile->push_back(std::move(packet));
// //                     }
// //                     packageStoredFile->flush();
// //                     sending_queue.clear();
// //                     ESP_LOGI(TAG, "Network offline: buffered packets flushed to Flash FIFO");
// //                 }
// //             } else {
// //                 // -------------------------------------------------------------
// //                 // ONLINE PATH:
// //                 // Step A: Drain disk FIFO first (Preserves chronological ordering)
// //                 // -------------------------------------------------------------
// //                 if (packageStoredFile && !packageStoredFile->empty()) {
// //                     packageStoredFile->begin_pop_mode();
// //                     size_t popped_count = 0;

// //                     while (!packageStoredFile->empty() && popped_count < MAX_DISK_BURST_PER_TICK) {
// //                         std::vector<uint8_t> disk_raw_packet = packageStoredFile->Pop();
// //                         popped_count++;

// //                         // Encrypt dynamically with the active session's aes_key
// //                         auto encrypted = encryption::aes_gcm_encrypt(disk_raw_packet, aes_key);
// //                         if (encrypted.has_value()) {
// //                             u_KcpClient->send(encrypted->data(), encrypted->size());
// //                         } else {
// //                             ESP_LOGE(TAG, "Failed to encrypt disk-restored packet");
// //                         }
// //                     }
// //                     packageStoredFile->flush();
// //                 }

// //                 // -------------------------------------------------------------
// //                 // Step B: Send real-time RAM packets from sending_queue
// //                 // -------------------------------------------------------------
// //                 if (!sending_queue.empty()) {
// //                     for (const auto& raw_packet : sending_queue) {
// //                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, aes_key);
// //                         if (encrypted.has_value()) {
// //                             int res = u_KcpClient->send(encrypted->data(), encrypted->size());
// //                             if (res == -1) {
// //                                 ESP_LOGW(TAG, "KCP send queue buffer full");
// //                             }
// //                         } else {
// //                             ESP_LOGE(TAG, "Failed to encrypt live packet");
// //                         }
// //                     }
// //                     sending_queue.clear();
// //                 }
// //             }

// //             // -----------------------------------------------------------------
// //             // 5. Task Cadence (Yield 10ms for KCP tick timer resolution)
// //             // -----------------------------------------------------------------
// //             vTaskDelay(pdMS_TO_TICKS(10));
// //         }

// //         vTaskDelete(nullptr);
// //     }

// // } // namespace Network


















// #include "Net manager.hpp"
// #include "KcpClient.hpp"
// #include "encryption/interface.hpp"

// #include <mutex>
// #include <queue>
// #include <atomic>
// #include <cstring>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_timer.h"
// #include "esp_random.h"

// namespace Network {

//     enum class HandshakeState {
//         IDLE,           // Wi-Fi down or uninitialized 
//         WAIT_ACK,       // Sent handshake, awaiting server response with retries
//         ESTABLISHED     // Handshake completed, AES key ready for normal traffic
//     };

//     // Internal state variables
//     static std::unique_ptr<KcpClient>   u_KcpClient;
//     static std::unique_ptr<file::FIFO>  packageStoredFile;
//     static uint32_t                     conv_id = 0;

//     static TaskHandle_t                 s_worker_task_handle = nullptr;
//     static std::atomic<bool>            s_worker_running{false};

//     // Queues
//     static std::mutex                               s_tx_mutex;
//     static std::vector<std::vector<uint8_t>>        incoming_queue;
//     static std::vector<std::vector<uint8_t>>        sending_queue;

//     static std::mutex                               s_rx_mutex;
//     static std::queue<std::vector<uint8_t>>         rx_queue;

//     // Handshake State Tracking
//     static HandshakeState               s_hs_state = HandshakeState::IDLE;
//     static std::vector<uint8_t>         s_cached_handshake_pkt; // Holds fixed packet for retries
//     static uint32_t                     s_last_handshake_attempt_ms = 0;
//     static uint32_t                     s_handshake_retry_count = 0;

//     // Retry configuration
//     static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Retry every 600ms
//     static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Cap backoff at 3s
//     static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;
//     static constexpr size_t   MAX_DISK_BURST_PER_TICK = 5;

//     static void network_worker_task(void* pvParameters);
//     static void handle_autonomous_handshake(uint32_t now_ms);

//     // =========================================================================
//     // Public APIs
//     // =========================================================================
//     void init(const char* host, const char* port) {
//         conv_id = esp_random();

//         packageStoredFile = std::make_unique<file::FIFO>("packageStoredFile");
//         u_KcpClient = std::make_unique<KcpClient>(conv_id);
//         u_KcpClient->initialize(host, port);

//         completeHandshake = false;
//         aes_key.clear();
//         s_hs_state = HandshakeState::IDLE;
//         s_cached_handshake_pkt.clear();

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

//         ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", conv_id);
//     }

//     void close_connection() {
//         s_worker_running.store(false);
//         s_worker_task_handle = nullptr;

//         if (u_KcpClient) {
//             u_KcpClient->close_connection();
//         }
//         s_hs_state = HandshakeState::IDLE;
//     }

//     int send(const std::vector<uint8_t>& data) {
//         if (data.empty()) return 0;

//         std::lock_guard<std::mutex> lock(s_tx_mutex);
//         if (incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
//             ESP_LOGW(TAG, "RAM queue full, packet dropped");
//             return -1;
//         }

//         incoming_queue.push_back(data);
//         return 0;
//     }

//     std::vector<uint8_t> poll_and_receive() {
//         std::lock_guard<std::mutex> lock(s_rx_mutex);
//         if (rx_queue.empty()) return {};

//         std::vector<uint8_t> data = std::move(rx_queue.front());
//         rx_queue.pop();
//         return data;
//     }

//     void update(uint32_t /*current_ms*/) {
//         // Managed internally inside the worker task
//     }

//     // =========================================================================
//     // Internal Handshake Engine (Called only within Worker Thread)
//     // =========================================================================
//     static void handle_autonomous_handshake(uint32_t now_ms) {
//         bool wifi_ok = wifi::isConnected();

//         // 1. If Wi-Fi dropped, reset handshake state machine
//         if (!wifi_ok) {
//             if (s_hs_state != HandshakeState::IDLE) {
//                 ESP_LOGW(TAG, "Wi-Fi lost. Resetting handshake state.");
//                 s_hs_state = HandshakeState::IDLE;
//                 completeHandshake = false;
//                 s_cached_handshake_pkt.clear();
//                 s_handshake_retry_count = 0;
//             }
//             return;
//         }

//         // 2. Wi-Fi is connected, progress through state machine
//         switch (s_hs_state) {
//             case HandshakeState::IDLE: {
//                 ESP_LOGI(TAG, "Wi-Fi active. Generating ECDH keys...");
//                 auto my_key_opt = encryption::ecdh_step1_generate_my_key();
//                 if (!my_key_opt.has_value()) {
//                     ESP_LOGE(TAG, "ECDH keygen failed, retrying next tick...");
//                     return;
//                 }

//                 const auto& pub_key_bytes = my_key_opt.value();
//                 s_cached_handshake_pkt.resize(sizeof(uint32_t) + pub_key_bytes.size());
                
//                 // Pack: [conv_id (4 bytes)] + [Client Public Key]
//                 std::memcpy(s_cached_handshake_pkt.data(), &conv_id, sizeof(uint32_t));
//                 std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

//                 // Initial transmission
//                 u_KcpClient->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
//                 s_last_handshake_attempt_ms = now_ms;
//                 s_handshake_retry_count = 1;
//                 s_hs_state = HandshakeState::WAIT_ACK;

//                 ESP_LOGI(TAG, "Handshake dispatched (attempt 1)");
//                 break;
//             }

//             case HandshakeState::WAIT_ACK: {
//                 // Compute retry backoff (e.g., 600ms, 1200ms, 1800ms... up to 3000ms)
//                 uint32_t current_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
//                 if (current_interval > MAX_HANDSHAKE_RETRY_MS) {
//                     current_interval = MAX_HANDSHAKE_RETRY_MS;
//                 }

//                 if (now_ms - s_last_handshake_attempt_ms >= current_interval) {
//                     s_handshake_retry_count++;
//                     ESP_LOGW(TAG, "Handshake ACK timeout. Retransmitting attempt %u...", s_handshake_retry_count);
                    
//                     // Resend the IDENTICAL packet (Never change the key!)
//                     u_KcpClient->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
//                     s_last_handshake_attempt_ms = now_ms;
//                 }
//                 break;
//             }

//             case HandshakeState::ESTABLISHED:
//                 // Nothing to do, handshake is already complete
//                 break;
//         }
//     }

//     // =========================================================================
//     // Consumer Worker Task
//     // =========================================================================
//     static void network_worker_task(void* /*pvParameters*/) {
//         while (s_worker_running.load()) {
//             uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

//             // 1. Maintain Autonomous Handshake (Send / Retry logic)
//             handle_autonomous_handshake(now_ms);

//             // 2. KCP Clock Update (Always ticks in this same thread)
//             if (u_KcpClient) {
//                 u_KcpClient->update(now_ms);
//             }

//             // 3. Receive & Process Inbound Packets
//             if (u_KcpClient) {
//                 auto incoming_data = u_KcpClient->poll_and_receive();
//                 if (!incoming_data.empty()) {
//                     if (s_hs_state == HandshakeState::WAIT_ACK) {
//                         // Received server's public key response!
//                         auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
//                         if (shared_secret.has_value()) {
//                             aes_key = shared_secret.value();
//                             completeHandshake = true;
//                             s_hs_state = HandshakeState::ESTABLISHED;
//                             s_cached_handshake_pkt.clear(); // Free memory

//                             ESP_LOGI(TAG, "Handshake completed successfully!");
//                         } else {
//                             ESP_LOGE(TAG, "Corrupt server handshake response, will continue retrying");
//                         }
//                     } else if (s_hs_state == HandshakeState::ESTABLISHED) {
//                         // Regular traffic: Decrypt application packet
//                         auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
//                         if (decrypted.has_value()) {
//                             std::lock_guard<std::mutex> lock(s_rx_mutex);
//                             rx_queue.push(std::move(decrypted.value()));
//                         }
//                     }
//                 }
//             }

//             // 4. Ping-Pong Swap Outbound Queue
//             {
//                 std::lock_guard<std::mutex> lock(s_tx_mutex);
//                 if (!incoming_queue.empty()) {
//                     sending_queue.swap(incoming_queue);
//                 }
//             }

//             // 5. Data Flow Routing
//             bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

//             if (!is_online) {
//                 // If offline or handshake in progress: Buffer new RAM packets into Flash FIFO
//                 if (!sending_queue.empty() && packageStoredFile) {
//                     packageStoredFile->begin_push_mode();
//                     for (auto& packet : sending_queue) {
//                         packageStoredFile->push_back(std::move(packet)); // Plain text
//                     }
//                     packageStoredFile->flush();
//                     sending_queue.clear();
//                 }
//             } else {
//                 // Drain Flash FIFO first (Oldest packets first)
//                 if (packageStoredFile && !packageStoredFile->empty()) {
//                     packageStoredFile->begin_pop_mode();
//                     size_t popped = 0;
//                     while (!packageStoredFile->empty() && popped < MAX_DISK_BURST_PER_TICK) {
//                         auto raw_disk_pkt = packageStoredFile->Pop();
//                         popped++;

//                         auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, aes_key);
//                         if (encrypted.has_value()) {
//                             u_KcpClient->send(encrypted->data(), encrypted->size());
//                         }
//                     }
//                     packageStoredFile->flush();
//                 }

//                 // Drain live RAM queue
//                 if (!sending_queue.empty()) {
//                     for (const auto& raw_packet : sending_queue) {
//                         auto encrypted = encryption::aes_gcm_encrypt(raw_packet, aes_key);
//                         if (encrypted.has_value()) {
//                             u_KcpClient->send(encrypted->data(), encrypted->size());
//                         }
//                     }
//                     sending_queue.clear();
//                 }
//             }

//             vTaskDelay(pdMS_TO_TICKS(10)); // 10ms KCP tick
//         }

//         vTaskDelete(nullptr);
//     }

// } // namespace Network














#include "Net manager.hpp"

// Internal Network & Storage Dependencies
#include "KcpClient.hpp"
#include "WiFiManager.hpp"
#include "FIFO_file.hpp"
#include "encryption/interface.hpp"

// System & FreeRTOS Headers
#include <mutex>
#include <queue>
#include <atomic>
#include <cstring>
#include <memory>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Network {

    static const char* TAG = "NETWORK_MANAGER";

    // Handshake State Engine
    enum class HandshakeState {
        IDLE,           // Offline or awaiting connection
        WAIT_ACK,       // UDP handshake packet dispatched, awaiting ECDH response
        ESTABLISHED     // Handshake complete, AES key active for live traffic
    };

    // =========================================================================
    // Static / Internal Handles
    // =========================================================================
    static std::unique_ptr<KcpClient>   s_kcp_client;
    static std::unique_ptr<file::FIFO>  s_disk_fifo;
    static uint32_t                     s_conv_id = 0;

    // Crypto & Handshake session state (Completely internal)
    static std::vector<uint8_t>         s_aes_key;
    static HandshakeState               s_hs_state = HandshakeState::IDLE;
    static std::vector<uint8_t>         s_cached_handshake_pkt; 
    static uint32_t                     s_last_handshake_attempt_ms = 0;
    static uint32_t                     s_handshake_retry_count = 0;

    // Worker Task Management
    static TaskHandle_t                 s_worker_task_handle = nullptr;
    static std::atomic<bool>            s_worker_running{false};

    // Ping-Pong Double-Buffering (Outbound)
    static std::mutex                               s_tx_mutex;
    static std::vector<std::vector<uint8_t>>        s_incoming_queue; // Written by caller threads
    static std::vector<std::vector<uint8_t>>        s_sending_queue;  // Processed by worker task

    // Inbound Rx Queue
    static std::mutex                               s_rx_mutex;
    static std::queue<std::vector<uint8_t>>         s_rx_queue;

    // Tuning Parameters
    static constexpr uint32_t HANDSHAKE_BASE_RETRY_MS = 600;  // Initial backoff window
    static constexpr uint32_t MAX_HANDSHAKE_RETRY_MS  = 3000; // Upper backoff ceiling
    static constexpr size_t   MAX_RAM_QUEUE_SIZE      = 128;  // RAM drop threshold
    static constexpr size_t   MAX_DISK_BURST_PER_TICK = 5;    // Max packets read from flash/tick

    // Forward declaration of internal task & helpers
    static void network_worker_task(void* pvParameters);
    static void handle_autonomous_handshake(uint32_t now_ms);

    // =========================================================================
    // Public API Implementation
    // =========================================================================

    void init(const char* host, const char* port) {
        s_conv_id = esp_random();

        // 1. Initialize persistent disk storage
        s_disk_fifo = std::make_unique<file::FIFO>("packageStoredFile");

        // 2. Initialize KCP instance
        s_kcp_client = std::make_unique<KcpClient>(s_conv_id);
        s_kcp_client->initialize(host, port);

        // 3. Reset internal handshake and session keys
        s_aes_key.clear();
        s_cached_handshake_pkt.clear();
        s_hs_state = HandshakeState::IDLE;

        // 4. Spawn the single sender/update worker task (Core 1)
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

        ESP_LOGI(TAG, "Network sender initialized (conv_id: %u)", s_conv_id);
    }

    void close_connection() {
        s_worker_running.store(false);
        s_worker_task_handle = nullptr;

        if (s_kcp_client) {
            s_kcp_client->close_connection();
        }

        s_hs_state = HandshakeState::IDLE;
        ESP_LOGI(TAG, "Network connection terminated");
    }

    int send(const std::vector<uint8_t>& data) {
        if (data.empty()) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(s_tx_mutex);
        if (s_incoming_queue.size() >= MAX_RAM_QUEUE_SIZE) {
            ESP_LOGW(TAG, "RAM queue full; message rejected (Backpressure)");
            return -1;
        }

        // Store unencrypted payload; background task will encrypt before transmission
        s_incoming_queue.push_back(data);
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

    // =========================================================================
    // Internal Autonomous Handshake Engine
    // =========================================================================

    static void handle_autonomous_handshake(uint32_t now_ms) {
        bool wifi_ok = wifi::isConnected();

        // If Wi-Fi is lost, cleanly reset the state machine
        if (!wifi_ok) {
            if (s_hs_state != HandshakeState::IDLE) {
                ESP_LOGW(TAG, "Wi-Fi disconnected: Handshake reset to IDLE");
                s_hs_state = HandshakeState::IDLE;
                s_cached_handshake_pkt.clear();
                s_handshake_retry_count = 0;
            }
            return;
        }

        // Wi-Fi is online; process state machine
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

                // Build initial frame: [conv_id (4 bytes)] + [Client Public Key]
                std::memcpy(s_cached_handshake_pkt.data(), &s_conv_id, sizeof(uint32_t));
                std::memcpy(s_cached_handshake_pkt.data() + sizeof(uint32_t), pub_key_bytes.data(), pub_key_bytes.size());

                // First UDP transmission
                s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
                s_last_handshake_attempt_ms = now_ms;
                s_handshake_retry_count = 1;
                s_hs_state = HandshakeState::WAIT_ACK;

                ESP_LOGI(TAG, "Initial handshake dispatched (conv_id: %u)", s_conv_id);
                break;
            }

            case HandshakeState::WAIT_ACK: {
                // Compute retry window with backoff cap
                uint32_t retry_interval = s_handshake_retry_count * HANDSHAKE_BASE_RETRY_MS;
                if (retry_interval > MAX_HANDSHAKE_RETRY_MS) {
                    retry_interval = MAX_HANDSHAKE_RETRY_MS;
                }

                if (now_ms - s_last_handshake_attempt_ms >= retry_interval) {
                    s_handshake_retry_count++;
                    ESP_LOGW(TAG, "Handshake ACK timed out; retransmitting attempt #%u...", s_handshake_retry_count);

                    // Retransmit the EXACT same packet (never regenerate keys on retry)
                    s_kcp_client->raw_udp_send(s_cached_handshake_pkt.data(), s_cached_handshake_pkt.size());
                    s_last_handshake_attempt_ms = now_ms;
                }
                break;
            }

            case HandshakeState::ESTABLISHED:
                // Handshake active; nothing to do
                break;
        }
    }

    // =========================================================================
    // Single Consumer Task (Handles KCP, Crypto, and Disk I/O)
    // =========================================================================

    static void network_worker_task(void* /*pvParameters*/) {
        while (s_worker_running.load()) {
            uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

            // 1. Maintain autonomous handshake retries
            handle_autonomous_handshake(now_ms);

            // 2. KCP Clock update (Runs in the exact same thread as send/receive)
            if (s_kcp_client) {
                s_kcp_client->update(now_ms);
            }

            // 3. Receive packets and process inbound server replies
            if (s_kcp_client) {
                auto incoming_data = s_kcp_client->poll_and_receive();
                if (!incoming_data.empty()) {
                    if (s_hs_state == HandshakeState::WAIT_ACK) {
                        // Received server's public key response
                        auto shared_secret = encryption::ecdh_step2_compute_shared_secret(incoming_data);
                        if (shared_secret.has_value()) {
                            s_aes_key = shared_secret.value();
                            s_hs_state = HandshakeState::ESTABLISHED;
                            s_cached_handshake_pkt.clear();

                            ESP_LOGI(TAG, "Handshake completed! Session AES key ready.");
                        } else {
                            ESP_LOGE(TAG, "Corrupt server response, continuing handshake retries");
                        }
                    } else if (s_hs_state == HandshakeState::ESTABLISHED) {
                        // Established session: decrypt payload and queue for caller
                        auto decrypted = encryption::aes_gcm_decrypt(incoming_data);
                        if (decrypted.has_value()) {
                            std::lock_guard<std::mutex> lock(s_rx_mutex);
                            s_rx_queue.push(std::move(decrypted.value()));
                        }
                    }
                }
            }

            // 4. Ping-Pong double-buffering swap
            {
                std::lock_guard<std::mutex> lock(s_tx_mutex);
                if (!s_incoming_queue.empty()) {
                    s_sending_queue.swap(s_incoming_queue);
                }
            }

            // 5. Check if pipeline is ready to transmit live over the network
            bool is_online = wifi::isConnected() && (s_hs_state == HandshakeState::ESTABLISHED);

            if (!is_online) {
                // OFFLINE PATH: Write plain data to flash FIFO (Keys change across reboots)
                if (!s_sending_queue.empty() && s_disk_fifo) {
                    s_disk_fifo->begin_push_mode();
                    for (auto& packet : s_sending_queue) {
                        s_disk_fifo->push_back(std::move(packet));
                    }
                    s_disk_fifo->flush();
                    s_sending_queue.clear();
                }
            } else {
                // ONLINE PATH:
                // Step A: Drain backlog from disk FIFO first (Preserve packet sequence)
                if (s_disk_fifo && !s_disk_fifo->empty()) {
                    s_disk_fifo->begin_pop_mode();
                    size_t popped = 0;
                    while (!s_disk_fifo->empty() && popped < MAX_DISK_BURST_PER_TICK) {
                        auto raw_disk_pkt = s_disk_fifo->Pop();
                        popped++;

                        // Encrypt on-the-fly with the active session's key
                        auto encrypted = encryption::aes_gcm_encrypt(raw_disk_pkt, s_aes_key);
                        if (encrypted.has_value()) {
                            s_kcp_client->send(encrypted->data(), encrypted->size());
                        }
                    }
                    s_disk_fifo->flush();
                }

                // Step B: Send pending RAM queue
                if (!s_sending_queue.empty()) {
                    for (const auto& raw_packet : s_sending_queue) {
                        auto encrypted = encryption::aes_gcm_encrypt(raw_packet, s_aes_key);
                        if (encrypted.has_value()) {
                            s_kcp_client->send(encrypted->data(), encrypted->size());
                        }
                    }
                    s_sending_queue.clear();
                }
            }

            // Yield 10ms for KCP tick interval
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        vTaskDelete(nullptr);
    }

} // namespace Network