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

void ToggleMicrophoneMute() {
	auto &calls = Core::App().calls();
	if (const auto call = calls->currentCall()) {
		call->setMuted(!call->muted());
	} else if (const auto group = calls->currentGroupCall()) {
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
