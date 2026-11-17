#pragma once

// Adapter: core::IMediaPlayer over the ESP-ADF simple player.

#include "interface.hpp"

#include "audio_driver.h"

namespace adapters {

class EspMediaPlayer : public core::IMediaPlayer {
public:
    bool init() override {
        Audio_Play_Init();
        return true;
    }

    bool play(const std::string& url) override {
        return Audio_Play_Music(url.c_str()) == ESP_GMF_ERR_OK;
    }

    void stop() override { Audio_Stop_Play(); }
    void pause() override { Audio_Pause_Play(); }
    void resume() override { Audio_Resume_Play(); }

    core::MediaState state() const override {
        switch (Audio_Get_Current_State()) {
            case ESP_ASP_STATE_RUNNING:  return core::MediaState::Running;
            case ESP_ASP_STATE_PAUSED:   return core::MediaState::Paused;
            case ESP_ASP_STATE_STOPPED:  return core::MediaState::Stopped;
            case ESP_ASP_STATE_FINISHED: return core::MediaState::Finished;
            case ESP_ASP_STATE_ERROR:    return core::MediaState::Error;
            default:                     return core::MediaState::None;
        }
    }

    uint8_t volume() const override { return get_audio_volume(); }
    void setVolume(uint8_t volume) override { Volume_Adjustment(volume); }
};

} // namespace adapters
