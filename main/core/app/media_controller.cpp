#include "core/app/media_controller.hpp"

namespace core::app {

MediaController::MediaController(core::IMediaPlayer& player,
                                 core::IIndicator& indicator,
                                 core::IButtonInput& buttons,
                                 core::ISpeechInput& speech,
                                 core::IPlaylist& playlist,
                                 core::ILogger& logger)
    : player_(player),
      indicator_(indicator),
      buttons_(buttons),
      speech_(speech),
      playlist_(playlist),
      logger_(logger) {}

bool MediaController::start(const MediaConfig& config) {
    config_ = config;

    if (!player_.init()) {
        logger_.logf(core::LogLevel::Error, "Media player init failed");
        return false;
    }

    tracks_ = playlist_.scan(config_.music_dir, config_.music_ext);
    logger_.logf(core::LogLevel::Info, "Media: %u track(s) found",
                 static_cast<unsigned>(tracks_.size()));

    indicator_.setMode(core::IndicatorMode::Idle);

    if (!buttons_.start(&MediaController::buttonTrampoline, this)) {
        logger_.logf(core::LogLevel::Warn, "Button input unavailable");
    }
    if (!speech_.start(&MediaController::speechTrampoline, this)) {
        logger_.logf(core::LogLevel::Warn, "Speech input unavailable");
    }
    return true;
}

void MediaController::buttonTrampoline(core::ButtonId id, core::ButtonEvent event, void* ctx) {
    if (auto* self = static_cast<MediaController*>(ctx)) {
        self->onButton(id, event);
    }
}

void MediaController::speechTrampoline(core::SpeechEvent event, int command, void* ctx) {
    if (auto* self = static_cast<MediaController*>(ctx)) {
        self->onSpeech(event, command);
    }
}

void MediaController::onButton(core::ButtonId id, core::ButtonEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    perform(actionFor(id, event));
}

core::app::MediaAction MediaController::actionFor(core::ButtonId id,
                                                  core::ButtonEvent event) const {
    for (const auto& binding : config_.bindings) {
        if (binding.id == id) {
            return (event == core::ButtonEvent::ShortPress) ? binding.short_action
                                                            : binding.long_action;
        }
    }
    return MediaAction::None;
}

void MediaController::perform(MediaAction action) {
    switch (action) {
        case MediaAction::VolumeUp:
            changeVolume(+static_cast<int>(config_.volume_step));
            break;
        case MediaAction::VolumeDown:
            changeVolume(-static_cast<int>(config_.volume_step));
            break;
        case MediaAction::TogglePlayPause:
            resume_after_voice_ = false;
            togglePlayPause();
            break;
        case MediaAction::Stop:
            resume_after_voice_ = false;
            player_.stop();
            indicator_.setMode(core::IndicatorMode::Idle);
            break;
        case MediaAction::NextTrack:
            resume_after_voice_ = false;
            if (!tracks_.empty()) {
                index_ = (index_ + 1) % tracks_.size();
                playCurrent();
            }
            break;
        case MediaAction::PreviousTrack:
            resume_after_voice_ = false;
            if (!tracks_.empty()) {
                index_ = (index_ + tracks_.size() - 1) % tracks_.size();
                playCurrent();
            }
            break;
        case MediaAction::None:
            break;
    }
}

void MediaController::togglePlayPause() {
    switch (player_.state()) {
        case core::MediaState::Running:
            player_.pause();
            indicator_.setMode(core::IndicatorMode::Idle);
            break;
        case core::MediaState::Paused:
            player_.resume();
            indicator_.setMode(core::IndicatorMode::Playing);
            break;
        case core::MediaState::Stopped:
        case core::MediaState::None:
            playCurrent();
            break;
        case core::MediaState::Finished:
            playCurrent();   // replay the finished track instead of sticking on Idle
            break;
        case core::MediaState::Error:
            playCurrent();   // retry instead of leaving the user stuck
            break;
    }
}

void MediaController::playCurrent() {
    if (tracks_.empty()) {
        logger_.logf(core::LogLevel::Warn, "Media: no tracks to play");
        return;
    }

    const std::string url = config_.url_prefix + tracks_[index_];
    if (!player_.play(url)) {
        logger_.logf(core::LogLevel::Error, "Media: failed to play %s", url.c_str());
        return;
    }

    indicator_.setMode(core::IndicatorMode::Playing);
    logger_.logf(core::LogLevel::Info, "Media: playing %s", url.c_str());
}

void MediaController::changeVolume(int delta) {
    int next = static_cast<int>(player_.volume()) + delta;
    if (next < 0) {
        next = 0;
    }
    if (next > static_cast<int>(config_.max_volume)) {
        next = config_.max_volume;
    }
    player_.setVolume(static_cast<uint8_t>(next));
}

void MediaController::onSpeech(core::SpeechEvent event, int command) {
    std::lock_guard<std::mutex> lock(mutex_);
    switch (event) {
        case core::SpeechEvent::Awake:
            if (player_.state() == core::MediaState::Running) {
                player_.pause();
                resume_after_voice_ = true;
            }
            indicator_.setMode(core::IndicatorMode::VoiceCommand);
            break;
        case core::SpeechEvent::Command:
            switch (command) {
                case 0: indicator_.setColor(core::IndicatorColor::Red); break;
                case 1: indicator_.setColor(core::IndicatorColor::Blue); break;
                case 2: indicator_.setColor(core::IndicatorColor::Green); break;
                case 3: indicator_.setColor(core::IndicatorColor::White); break;
                default: break;
            }
            break;
        case core::SpeechEvent::CommandTimeout:
            if (resume_after_voice_) {
                player_.resume();
                indicator_.setMode(core::IndicatorMode::Playing);
                resume_after_voice_ = false;
            } else {
                indicator_.setMode(core::IndicatorMode::Idle);
            }
            break;
    }
}

} // namespace core::app
