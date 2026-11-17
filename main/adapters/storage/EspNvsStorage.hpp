#pragma once

// Adapter: ESP-IDF NVS implementation of the core::IStorage port.

#include "core/ports.hpp"

#include <string>
#include <utility>

#include "nvs.h"
#include "nvs_flash.h"

namespace adapters {

class EspNvsStorage : public core::IStorage {
public:
    explicit EspNvsStorage(std::string namespace_name)
        : namespace_(std::move(namespace_name)) {}

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

        size_t len = 0;
        esp_err_t err = nvs_get_blob(h, key.c_str(), nullptr, &len);
        if (err != ESP_OK) {
            nvs_close(h);
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
        esp_err_t err = nvs_get_blob(h, key.c_str(), nullptr, &len);
        nvs_close(h);
        return err == ESP_OK;
    }

private:
    std::string namespace_;
};

} // namespace adapters
