#pragma once

// Adapter: ESP-IDF NVS implementation of the core::IStorage port.

#include "interface.hpp"

#include <string>
#include <utility>

#include "nvs.h"
#include "nvs_flash.h"

namespace adapters {

class EspNvsStorage : public core::IStorage {
public:
    explicit EspNvsStorage(std::string namespace_name)
        : namespace_(std::move(namespace_name)) {
        // Idempotent: safe when the WiFi manager (or anyone else) initialized NVS
        // first. Handles the two states that require a full erase.
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }
    }

    bool set(const std::string& key, const core::Bytes& value) override {
        nvs_handle_t h;
        if (nvs_open(namespace_.c_str(), NVS_READWRITE, &h) != ESP_OK) {
            return false;
        }
        bool ok = (nvs_set_blob(h, key.c_str(), value.data(), value.size()) == ESP_OK);
        ok = ok && (nvs_commit(h) == ESP_OK);
        nvs_close(h);
        return ok;
    }

    bool get(const std::string& key, core::Bytes& out) override {
        nvs_handle_t h;
        if (nvs_open(namespace_.c_str(), NVS_READONLY, &h) != ESP_OK) {
            return false;
        }

        // Provisioned values may be stored as NVS strings (device_id from
        // provision_tool --nvs-csv is "data,string"). In ESP-IDF 6.x a blob
        // lookup on a string key returns ESP_ERR_NVS_NOT_FOUND, not
        // ESP_ERR_NVS_TYPE_MISMATCH, so try the string getter first and fall
        // back to the blob getter only if the key is not a string.
        size_t len = 0;
        esp_err_t err = nvs_get_str(h, key.c_str(), nullptr, &len);
        if (err == ESP_OK && len > 0) {
            out.resize(len);
            err = nvs_get_str(h, key.c_str(),
                              reinterpret_cast<char*>(out.data()), &len);
            nvs_close(h);
            if (err == ESP_OK) {
                out.resize(len - 1);   // drop the NUL terminator
                return true;
            }
            out.clear();
            return false;
        }

        len = 0;
        err = nvs_get_blob(h, key.c_str(), nullptr, &len);
        if (err != ESP_OK) {
            nvs_close(h);
            out.clear();
            return false;
        }

        out.resize(len);
        err = nvs_get_blob(h, key.c_str(), out.data(), &len);
        nvs_close(h);

        if (err != ESP_OK) {
            out.clear();
            return false;
        }
        out.resize(len);
        return true;
    }

    bool remove(const std::string& key) override {
        nvs_handle_t h;
        if (nvs_open(namespace_.c_str(), NVS_READWRITE, &h) != ESP_OK) {
            return false;
        }
        bool ok = (nvs_erase_key(h, key.c_str()) == ESP_OK);
        ok = ok && (nvs_commit(h) == ESP_OK);
        nvs_close(h);
        return ok;
    }

    bool exists(const std::string& key) override {
        nvs_handle_t h;
        if (nvs_open(namespace_.c_str(), NVS_READONLY, &h) != ESP_OK) {
            return false;
        }
        size_t len = 0;
        if (nvs_get_str(h, key.c_str(), nullptr, &len) == ESP_OK) {
            nvs_close(h);
            return true;
        }
        const esp_err_t err = nvs_get_blob(h, key.c_str(), nullptr, &len);
        nvs_close(h);
        return err == ESP_OK;
    }

private:
    std::string namespace_;
};

} // namespace adapters
