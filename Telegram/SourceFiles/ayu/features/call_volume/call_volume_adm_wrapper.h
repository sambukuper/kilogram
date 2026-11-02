// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "webrtc/webrtc_create_adm.h"

#include <atomic>
#include <memory>
#include <functional>

namespace Ayu {

void ApplyVolumeWithLimiter(int16_t *samples, size_t count, float gain);

[[nodiscard]] Webrtc::AudioDeviceModulePtr WrapAudioDeviceModule(
	Webrtc::AudioDeviceModulePtr impl,
	std::shared_ptr<std::atomic<float>> gain);

[[nodiscard]] auto WrapAudioDeviceModuleCreator(
	std::function<Webrtc::AudioDeviceModulePtr(webrtc::TaskQueueFactory*)> creator,
	std::shared_ptr<std::atomic<float>> gain)
-> std::function<Webrtc::AudioDeviceModulePtr(webrtc::TaskQueueFactory*)>;

} // namespace Ayu
