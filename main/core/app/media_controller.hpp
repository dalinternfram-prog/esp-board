#pragma once

// Application media behaviour: playlist navigation, volume, play/pause/stop and
// voice-command feedback. Pure C++ built only on core ports.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "interface.hpp"

namespace core::app {

enum class MediaAction {
    None,
    VolumeUp,
    VolumeDown,
    TogglePlayPause,
    Stop,
    NextTrack,
    PreviousTrack,
};

struct ButtonBinding {
    core::ButtonId id;
    MediaAction    short_action;
    MediaAction    long_action;
};

struct MediaConfig {
    std::string url_prefix = "file://sdcard/";
    std::string music_dir  = "/sdcard";
    std::string music_ext  = ".mp3";

    uint8_t max_volume  = 100;
    uint8_t volume_step = 10;

    // Board key layout: which physical key does what.
    ButtonBinding bindings[3] = {
        {core::ButtonId::Previous,  MediaAction::VolumeUp,        MediaAction::PreviousTrack},
        {core::ButtonId::PlayPause, MediaAction::TogglePlayPause, MediaAction::Stop},
        {core::ButtonId::Next,      MediaAction::VolumeDown,      MediaAction::NextTrack},
    };
};

class MediaController {
public:
    MediaController(core::IMediaPlayer& player,
                    core::IIndicator& indicator,
                    core::IButtonInput& buttons,
                    core::ISpeechInput& speech,
                    core::IPlaylist& playlist,
                    core::ILogger& logger);

    bool start(const MediaConfig& config);

private:
    static void buttonTrampoline(core::ButtonId id, core::ButtonEvent event, void* ctx);
    static void speechTrampoline(core::SpeechEvent event, int command, void* ctx);

    void onButton(core::ButtonId id, core::ButtonEvent event);
    void onSpeech(core::SpeechEvent event, int command);
    void perform(MediaAction action);
    void playCurrent();
    void togglePlayPause();
    void changeVolume(int delta);
    MediaAction actionFor(core::ButtonId id, core::ButtonEvent event) const;

    core::IMediaPlayer& player_;
    core::IIndicator&   indicator_;
    core::IButtonInput& buttons_;
    core::ISpeechInput& speech_;
    core::IPlaylist&    playlist_;
    core::ILogger&      logger_;

    MediaConfig config_;
    std::vector<std::string> tracks_;
    std::size_t index_ = 0;
    bool resume_after_voice_ = false;
};

} // namespace core::app
