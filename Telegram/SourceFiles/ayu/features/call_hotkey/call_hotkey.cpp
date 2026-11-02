// This is the source code of Kilogram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
#include "ayu/features/call_hotkey/call_hotkey.h"

#include "ayu/ayu_settings.h"
#include "base/global_shortcuts.h"
#include "calls/calls_call.h"
#include "calls/calls_instance.h"
#include "calls/group/calls_group_call.h"
#include "core/application.h"
#include "media/audio/media_audio_track.h"

namespace AyuFeatures::CallHotkey {
namespace {

rpl::lifetime WatcherLifetime;

std::shared_ptr<base::GlobalShortcutManager> &ManagerRef() {
	static std::shared_ptr<base::GlobalShortcutManager> manager;
	return manager;
}

base::GlobalShortcut &ShortcutRef() {
	static base::GlobalShortcut shortcut;
	return shortcut;
}

[[nodiscard]] QByteArray DeserializeSetting(const QString &value) {
	return QByteArray::fromBase64(value.toUtf8());
}

} // namespace

std::shared_ptr<base::GlobalShortcutManager> Manager() {
	auto &manager = ManagerRef();
	if (!manager) {
		manager = base::CreateGlobalShortcutManager();
	}
	return manager;
}

void PlayMuteSound(bool muted) {
	crl::on_main([=] {
		const auto &settings = AyuSettings::getInstance();
		if (!settings.callMuteSoundEnabled()) {
			return;
		}
		const auto volume = settings.callMuteSoundVolume();
		if (volume <= 0) {
			return;
		}
		static std::unique_ptr<Media::Audio::Track> activatedTrack;
		static std::unique_ptr<Media::Audio::Track> mutedTrack;
		static auto hooked = false;
		if (!hooked) {
			hooked = true;
			Core::App().lifetime().add([] {
				activatedTrack.reset();
				mutedTrack.reset();
			});
		}
		auto &track = muted ? mutedTrack : activatedTrack;
		if (!track) {
			track = Media::Audio::Current().createTrack();
			track->fillFromFile(muted
				? u":/sounds/mic_muted.mp3"_q
				: u":/sounds/mic_activated.mp3"_q);
		}
		const auto gain = std::clamp(float64(volume) / 100.0, 0.001, 1.0);
		track->playOnce(gain);
	});
}

void ToggleMicrophoneMute() {
	auto &calls = Core::App().calls();
	if (const auto call = calls.currentCall()) {
		call->setMuted(!call->muted());
	} else if (const auto group = calls.currentGroupCall()) {
		group->setMutedAndUpdate(
			(group->muted() == Calls::MuteState::Muted)
				? Calls::MuteState::Active
				: Calls::MuteState::Muted);
	}
}

void ApplySetting() {
	const auto serialized = DeserializeSetting(
		AyuSettings::getInstance().callMuteHotkey());
	auto &manager = ManagerRef();
	auto &shortcut = ShortcutRef();
	if (serialized.isEmpty()
		|| !base::GlobalShortcutsAvailable()
		|| !base::GlobalShortcutsAllowed()) {
		shortcut = nullptr;
		manager = nullptr;
		return;
	}
	auto parsed = Manager()->shortcutFromSerialized(serialized);
	if (!parsed) {
		shortcut = nullptr;
		manager = nullptr;
		return;
	}
	if (shortcut && (parsed->serialize() == shortcut->serialize())) {
		return;
	}
	if (shortcut) {
		manager->stopWatching(shortcut);
	}
	shortcut = parsed;
	manager->startWatching(shortcut, [](bool pressed) {
		if (pressed) {
			ToggleMicrophoneMute();
		}
	});
}

void Init() {
	AyuSettings::getInstance().callMuteHotkeyChanges(
	) | rpl::on_next([=] {
		ApplySetting();
	}, WatcherLifetime);
	ApplySetting();
}

} // namespace AyuFeatures::CallHotkey
