// #include "file_manager1.1.hpp" // Import the FileManager declarations and standard types.
// #include "esp_littlefs.h" // Import the ESP-IDF LittleFS mount and statistics APIs.
// #include "esp_log.h" // Import ESP-IDF logging macros.
// #include <algorithm> // Import std::min for bounded chunk sizes.
// #include <cerrno> // Import errno for filesystem error diagnostics.
// #include <cstdio> // Import FILE, fopen, fread, fwrite, fseek, ftell, fclose, and fileno.
// #include <limits> // Import numeric_limits for safe size conversions.
// #include <sys/stat.h> // Import stat and struct stat.
// #include <sys/types.h> // Import off_t and filesystem type declarations.
// #include <unistd.h> // Import unlink and ftruncate.

// namespace FileManager { // Keep implementation symbols inside the FileManager namespace.

// static const char* TAG = "FileManager"; // Define the log tag used by this module.
// static std::string s_base_path = "/littlefs"; // Store the configured VFS mount path.
// static std::string s_partition_label = "my_files"; // Store the LittleFS partition label.
// static bool s_is_mounted = false; // Track whether this module successfully mounted LittleFS.
// static std::recursive_mutex s_mutex; // Serialize all FileManager calls; recursion is needed by shiftBytes and init helpers.

// static std::string getFullPath(const std::string& filename) { // Convert a caller path into a mounted VFS path.
//     if (filename.empty()) return s_base_path; // Treat an empty name as the mount directory, matching the previous implementation.
//     if (filename[0] == '/') return s_base_path + filename; // Preserve the prior behavior of treating a leading slash as mount-relative.
//     return s_base_path + "/" + filename; // Add a slash between the mount path and a relative file name.
// } // Finish getFullPath.

// static bool writeBytesUnlocked(const std::string& filename, const uint8_t* data, size_t dataSize, long offset) { // Write bytes while the caller holds s_mutex.
//     if (!s_is_mounted) { ESP_LOGE(TAG, "Write requested while LittleFS is not mounted"); return false; } // Reject file access before initialization or after deinitialization.
//     if (dataSize > 0 && data == nullptr) { ESP_LOGE(TAG, "Null data pointer for non-empty write"); return false; } // Avoid dereferencing a null pointer for non-empty data.
//     const std::string path = getFullPath(filename); // Resolve the full VFS path while mount configuration is protected by the mutex.
//     FILE* file = fopen(path.c_str(), "r+b"); // Open an existing file for binary read/write without truncating it.
//     if (file == nullptr) file = fopen(path.c_str(), "w+b"); // Create a missing file, truncating only the newly created path.
//     if (file == nullptr) { ESP_LOGE(TAG, "Open for writing failed: %s (errno=%d)", path.c_str(), errno); return false; } // Report failure if neither open mode worked.
//     if (fseek(file, 0, SEEK_END) != 0) { ESP_LOGE(TAG, "Seek to EOF failed: %s", path.c_str()); fclose(file); return false; } // Move to EOF to discover the current file length.
//     const long fileSize = ftell(file); // Read the current file length from the stream position.
//     if (fileSize < 0) { ESP_LOGE(TAG, "ftell failed: %s", path.c_str()); fclose(file); return false; } // Reject an invalid file length.
//     if (offset < 0) offset = fileSize; // Interpret every negative offset as append mode.
//     if (offset > fileSize) { // Fill a requested gap when the explicit offset lies beyond EOF.
//         constexpr size_t ZERO_BUFFER_SIZE = 256; // Bound temporary padding memory to a small fixed buffer.
//         const uint8_t zeros[ZERO_BUFFER_SIZE] = {}; // Initialize a reusable zero-filled padding buffer.
//         long remaining = offset - fileSize; // Calculate how many zero bytes are needed before the target offset.
//         while (remaining > 0) { // Write padding in bounded chunks until the gap is filled.
//             const size_t amount = static_cast<size_t>(std::min<long>(remaining, static_cast<long>(ZERO_BUFFER_SIZE))); // Select the smaller of the remaining gap and buffer capacity.
//             if (fwrite(zeros, 1, amount, file) != amount) { ESP_LOGE(TAG, "Zero-padding write failed: %s", path.c_str()); fclose(file); return false; } // Stop if the padding write is incomplete.
//             remaining -= static_cast<long>(amount); // Account for the successfully written padding chunk.
//         } // Finish zero-padding loop.
//     } // Finish gap-padding branch.
//     if (fseek(file, offset, SEEK_SET) != 0) { ESP_LOGE(TAG, "Seek to offset %ld failed: %s", offset, path.c_str()); fclose(file); return false; } // Position the stream at the requested write offset.
//     const size_t written = (dataSize == 0) ? 0 : fwrite(data, 1, dataSize, file); // Write exactly dataSize bytes without text conversion.
//     const bool flushOk = (fflush(file) == 0); // Flush C-library buffered data to the VFS before closing the file.
//     const bool closeOk = (fclose(file) == 0); // Close the stream and capture any reported close-time error.
//     if (written != dataSize || !flushOk || !closeOk) { ESP_LOGE(TAG, "Write/flush/close failed: %s (written=%u expected=%u)", path.c_str(), static_cast<unsigned>(written), static_cast<unsigned>(dataSize)); return false; } // Report any short write or stream error.
//     return true; // Report success only after the complete write, flush, and close succeed.
// } // Finish writeBytesUnlocked.

// esp_err_t init(const std::string& base_path, const std::string& partition_label) { // Mount LittleFS and register its VFS path.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Prevent concurrent mount-state changes or file operations during initialization.
//     if (s_is_mounted) { // Handle repeated init calls without mounting the same path twice.
//         if (s_base_path == base_path && s_partition_label == partition_label) return ESP_OK; // Treat a repeated identical initialization as success.
//         ESP_LOGE(TAG, "LittleFS already mounted at %s using partition %s", s_base_path.c_str(), s_partition_label.c_str()); // Explain why changing the mount configuration is rejected.
//         return ESP_ERR_INVALID_STATE; // Require callers to deinitialize before changing mount configuration.
//     } // Finish repeated-init handling.
//     s_base_path = base_path; // Save the requested VFS mount path.
//     s_partition_label = partition_label; // Save the requested partition label.
//     ESP_LOGI(TAG, "Mounting LittleFS partition [%s] at [%s]", s_partition_label.c_str(), s_base_path.c_str()); // Log the mount request.
//     esp_vfs_littlefs_conf_t conf = {}; // Zero-initialize all configuration fields supported by the installed component version.
//     conf.base_path = s_base_path.c_str(); // Set the VFS mount prefix.
//     conf.partition_label = s_partition_label.c_str(); // Select the partition from the partition table.
//     conf.format_if_mount_failed = false; // Never automatically erase recordings if mounting fails.
//     conf.dont_mount = false; // Request a normal mounted filesystem rather than registration-only behavior.
//     const esp_err_t result = esp_vfs_littlefs_register(&conf); // Register and mount the LittleFS partition.
//     if (result != ESP_OK) { // Handle mount failure without marking the filesystem active.
//         ESP_LOGE(TAG, "LittleFS mount failed for partition '%s': %s", s_partition_label.c_str(), esp_err_to_name(result)); // Report the component error.
//         return result; // Return the original mount error to the caller.
//     } // Finish mount-failure handling.
//     s_is_mounted = true; // Mark the filesystem active only after successful registration.
//     size_t total = 0; // Initialize the total-capacity output.
//     size_t free = 0; // Initialize the free-capacity output.
//     if (getStorageStats(&total, &free)) ESP_LOGI(TAG, "LittleFS mounted: total=%u bytes free=%u bytes", static_cast<unsigned>(total), static_cast<unsigned>(free)); // Log capacity using the recursively locked public statistics function.
//     return ESP_OK; // Report successful initialization.
// } // Finish init.

// esp_err_t deinit() { // Unmount LittleFS and unregister its VFS path.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Prevent unmounting while another FileManager operation is in progress.
//     if (!s_is_mounted) return ESP_OK; // Make repeated deinitialization harmless.
//     const esp_err_t result = esp_vfs_littlefs_unregister(s_partition_label.c_str()); // Unregister the mounted partition.
//     if (result == ESP_OK) { // Update state only if unregistration succeeded.
//         s_is_mounted = false; // Mark the filesystem inactive.
//         ESP_LOGI(TAG, "LittleFS unmounted successfully"); // Record successful unmounting.
//     } else { // Preserve mounted state if the component reports a failure.
//         ESP_LOGE(TAG, "LittleFS unmount failed: %s", esp_err_to_name(result)); // Report the unmount error.
//     } // Finish unmount result handling.
//     return result; // Return the component's status.
// } // Finish deinit.

// bool writeFile(const std::string& filename, const std::string& data, long offset) { // Write the contents of a string using the shared byte writer.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Serialize this write with all other FileManager operations.
//     return writeBytesUnlocked(filename, reinterpret_cast<const uint8_t*>(data.data()), data.size(), offset); // Preserve exact string length, including embedded NUL bytes.
// } // Finish string writeFile.

// bool writeFile(const std::string& filename, const std::vector<uint8_t>& data, long offset) { // Write a vector without allocating an intermediate std::string.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Serialize this write with all other FileManager operations.
//     return writeBytesUnlocked(filename, data.data(), data.size(), offset); // Write the original binary buffer directly.
// } // Finish vector writeFile.

// std::vector<uint8_t> readFileBytes(const std::string& filename, long offset, long size) { // Read a bounded region of a file.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Prevent races with FileManager writes, shifts, truncation, and deletion.
//     if (!s_is_mounted) { ESP_LOGE(TAG, "Read requested while LittleFS is not mounted"); return {}; } // Reject reads before mount.
//     if (offset < 0) { ESP_LOGE(TAG, "Negative read offset is invalid"); return {}; } // Require a non-negative read offset.
//     const std::string path = getFullPath(filename); // Resolve the path under the shared lock.
//     FILE* file = fopen(path.c_str(), "rb"); // Open the file in binary read-only mode.
//     if (file == nullptr) { ESP_LOGW(TAG, "Open for reading failed: %s (errno=%d)", path.c_str(), errno); return {}; } // Return an empty vector if the file cannot be opened.
//     if (fseek(file, 0, SEEK_END) != 0) { ESP_LOGE(TAG, "Seek to EOF failed: %s", path.c_str()); fclose(file); return {}; } // Seek to EOF to determine the file length.
//     const long totalLength = ftell(file); // Retrieve the file length.
//     if (totalLength < 0 || offset >= totalLength) { fclose(file); return {}; } // Preserve the previous behavior of returning empty for EOF or out-of-range offsets.
//     const size_t available = static_cast<size_t>(totalLength - offset); // Calculate remaining bytes without adding potentially overflowing values.
//     const size_t requested = (size < 0) ? available : std::min(static_cast<size_t>(size), available); // Clamp the requested length to bytes remaining in the file.
//     std::vector<uint8_t> content(requested); // Allocate exactly the number of bytes requested from the file.
//     if (fseek(file, offset, SEEK_SET) != 0) { ESP_LOGE(TAG, "Seek to offset %ld failed: %s", offset, path.c_str()); fclose(file); return {}; } // Move to the requested starting byte.
//     const size_t bytesRead = requested == 0 ? 0 : fread(content.data(), 1, requested, file); // Read the requested number of bytes.
//     const bool readOk = !ferror(file); // Capture the stream error state before closing.
//     if (fclose(file) != 0) ESP_LOGW(TAG, "Close after read reported an error: %s", path.c_str()); // Report a close error without discarding already-read data.
//     if (!readOk) { ESP_LOGE(TAG, "Read failed: %s", path.c_str()); return {}; } // Return empty on an actual stream error.
//     content.resize(bytesRead); // Trim the vector if EOF caused a short read.
//     return content; // Return the bytes successfully read.
// } // Finish readFileBytes.

// bool truncate(const std::string& filename, size_t newSize) { // Resize a file to the requested length.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Prevent concurrent FileManager operations while resizing.
//     if (!s_is_mounted) { ESP_LOGE(TAG, "Truncate requested while LittleFS is not mounted"); return false; } // Reject operations before mount.
//     if (newSize > static_cast<size_t>(std::numeric_limits<off_t>::max())) { ESP_LOGE(TAG, "Requested truncate size is not representable"); return false; } // Avoid narrowing a size that off_t cannot represent.
//     const std::string path = getFullPath(filename); // Resolve the path under the shared lock.
//     FILE* file = fopen(path.c_str(), "r+b"); // Open an existing file without truncating it on open.
//     if (file == nullptr) { ESP_LOGE(TAG, "Open for truncate failed: %s (errno=%d)", path.c_str(), errno); return false; } // Do not create a missing file implicitly.
//     const int resizeResult = ftruncate(fileno(file), static_cast<off_t>(newSize)); // Resize the file using the underlying POSIX file descriptor.
//     const bool closeOk = (fclose(file) == 0); // Close the stream and capture a reported close error.
//     if (resizeResult != 0 || !closeOk) { ESP_LOGE(TAG, "Truncate failed: %s (errno=%d)", path.c_str(), errno); return false; } // Report resize or close failure.
//     return true; // Report successful resizing.
// } // Finish truncate.

// void shiftBytes(const std::string& filename, size_t shiftDistance, bool left, long long startOffset, long long endOffset) { // Move a region in bounded chunks while holding the module lock.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Keep other FileManager operations out of this multi-step transformation.
//     if (!s_is_mounted || shiftDistance == 0) return; // Skip work when unmounted or when no shift is requested.
//     if (shiftDistance > static_cast<size_t>(std::numeric_limits<long>::max())) { ESP_LOGE(TAG, "Shift distance is too large"); return; } // Avoid overflow when converting the shift distance to long.
//     const std::string path = getFullPath(filename); // Resolve the path once for file-size inspection.
//     FILE* file = fopen(path.c_str(), "rb"); // Open the file for determining its current size.
//     if (file == nullptr) { ESP_LOGE(TAG, "Open for shift failed: %s", path.c_str()); return; } // Stop if the target file cannot be opened.
//     if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return; } // Seek to EOF before measuring the file.
//     const long fileSize = ftell(file); // Retrieve the current file size.
//     fclose(file); // Close the size-inspection stream before using the public read/write helpers.
//     if (fileSize <= 0) return; // No bytes can be shifted in an empty or invalid-size file.
//     const long actualStart = (startOffset < 0) ? 0 : static_cast<long>(std::min<long long>(startOffset, std::numeric_limits<long>::max())); // Clamp the start offset to a representable non-negative long.
//     const long actualEnd = (endOffset < 0 || endOffset > fileSize) ? fileSize : static_cast<long>(endOffset); // Treat endOffset as an exclusive boundary and clamp it to EOF.
//     if (actualStart >= fileSize || actualStart >= actualEnd) return; // Skip an empty or invalid region.
//     constexpr size_t CHUNK_SIZE = 2048; // Limit temporary RAM usage to 2 KiB per chunk.
//     if (left) { // Shift the selected region toward lower file offsets.
//         long currentSrc = actualStart; // Start reading at the beginning of the region.
//         long currentDst = actualStart - static_cast<long>(shiftDistance); // Calculate the first destination offset.
//         if (currentDst < 0) currentDst = 0; // Preserve the previous behavior of clamping a negative destination to byte zero.
//         while (currentSrc < actualEnd) { // Process the selected range from front to back to avoid overwriting unread source bytes.
//             const size_t amount = std::min(CHUNK_SIZE, static_cast<size_t>(actualEnd - currentSrc)); // Limit this read to the remaining region or buffer capacity.
//             const std::vector<uint8_t> chunk = readFileBytes(filename, currentSrc, static_cast<long>(amount)); // Read one source chunk using the recursively locked public API.
//             if (chunk.empty()) { ESP_LOGE(TAG, "Read failed during left shift: %s", path.c_str()); break; } // Stop if no bytes could be read.
//             if (!writeBytesUnlocked(filename, chunk.data(), chunk.size(), currentDst)) { ESP_LOGE(TAG, "Write failed during left shift: %s", path.c_str()); break; } // Write the chunk to its destination without releasing the outer lock.
//             currentSrc += static_cast<long>(chunk.size()); // Advance the source to the next unread byte.
//             currentDst += static_cast<long>(chunk.size()); // Advance the destination by the same amount.
//         } // Finish the left-shift loop.
//     } else { // Shift the selected region toward higher file offsets.
//         long currentSrc = actualEnd; // Begin at the exclusive end of the source region.
//         while (currentSrc > actualStart) { // Process backwards so destination writes do not overwrite unread source bytes.
//             const size_t amount = std::min(CHUNK_SIZE, static_cast<size_t>(currentSrc - actualStart)); // Select the next chunk size.
//             currentSrc -= static_cast<long>(amount); // Move to the beginning of the next source chunk.
//             const std::vector<uint8_t> chunk = readFileBytes(filename, currentSrc, static_cast<long>(amount)); // Read the source chunk.
//             if (chunk.empty()) { ESP_LOGE(TAG, "Read failed during right shift: %s", path.c_str()); break; } // Stop if no bytes could be read.
//             if (currentSrc > std::numeric_limits<long>::max() - static_cast<long>(shiftDistance)) { ESP_LOGE(TAG, "Destination offset overflow during right shift"); break; } // Prevent signed overflow when computing the destination.
//             const long targetDst = currentSrc + static_cast<long>(shiftDistance); // Calculate the destination offset for this chunk.
//             if (!writeBytesUnlocked(filename, chunk.data(), chunk.size(), targetDst)) { ESP_LOGE(TAG, "Write failed during right shift: %s", path.c_str()); break; } // Write the chunk without releasing the outer lock.
//         } // Finish the right-shift loop.
//     } // Finish direction selection.
// } // Finish shiftBytes.

// bool deleteFile(const std::string& filename) { // Delete a file from the mounted filesystem.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Serialize deletion with all other FileManager operations.
//     if (!s_is_mounted) return false; // Reject deletion before mount.
//     const std::string path = getFullPath(filename); // Resolve the target path.
//     if (unlink(path.c_str()) != 0) { ESP_LOGE(TAG, "Delete failed: %s (errno=%d)", path.c_str(), errno); return false; } // Report unlink failure.
//     return true; // Report successful deletion.
// } // Finish deleteFile.

// bool exists(const std::string& filename) { // Check whether a file or filesystem object exists.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Prevent races with FileManager deletion and mount changes.
//     if (!s_is_mounted) return false; // Report false when the filesystem is unavailable.
//     const std::string path = getFullPath(filename); // Resolve the path before stat().
//     struct stat info = {}; // Initialize the metadata structure before passing it to stat().
//     return stat(path.c_str(), &info) == 0; // Return true when stat succeeds.
// } // Finish exists.

// bool getStorageStats(size_t* total_bytes, size_t* free_bytes) { // Query total and available LittleFS space.
//     std::lock_guard<std::recursive_mutex> lock(s_mutex); // Protect mount state and partition label while querying statistics.
//     if (!s_is_mounted || total_bytes == nullptr || free_bytes == nullptr) return false; // Reject unmounted access or null output pointers.
//     size_t total = 0; // Initialize the component's total-capacity output.
//     size_t used = 0; // Initialize the component's used-capacity output.
//     const esp_err_t result = esp_littlefs_info(s_partition_label.c_str(), &total, &used); // Ask LittleFS for total and used bytes.
//     if (result != ESP_OK) { ESP_LOGE(TAG, "LittleFS statistics failed: %s", esp_err_to_name(result)); return false; } // Report component failure.
//     *total_bytes = total; // Return total partition capacity to the caller.
//     *free_bytes = (total > used) ? (total - used) : 0; // Calculate free capacity without unsigned underflow.
//     return true; // Report successful statistics retrieval.
// } // Finish getStorageStats.

// } // namespace FileManager // End the FileManager namespace.
