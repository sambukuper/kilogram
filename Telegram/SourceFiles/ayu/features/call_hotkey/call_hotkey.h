// This is the source code of Kilogram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
#pragma once

#include "base/global_shortcuts.h"

namespace AyuFeatures::CallHotkey {

// Observes the configured hotkey in AyuSettings and (re)registers
// a global shortcut watcher. Call once at application startup.
void Init();

// Re-reads the hotkey settings and (re)applies the global watchers.
void ApplySetting();

// Toggles microphone mute of the currently active call
// (regular or group). Does nothing when there is no call.
void ToggleMicrophoneMute();

// Toggles deafen of the currently active regular call.
// Does nothing when there is no call.
void ToggleDeafen();

// The shared global shortcut manager used by the feature,
// also used by the settings recording UI.
[[nodiscard]] std::shared_ptr<base::GlobalShortcutManager> Manager();

// Plays voice audio indicator (TeamSpeak style) for microphone
// mute (muted = true) or unmute/activated (muted = false).
void PlayMuteSound(bool muted);

} // namespace AyuFeatures::CallHotkey
