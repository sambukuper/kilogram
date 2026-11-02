// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <modules/audio_device/include/audio_device.h>
#include <api/scoped_refptr.h>
#include <atomic>
#include <memory>
#include <functional>

namespace Ayu {

void ApplyVolumeWithLimiter(int16_t *samples, size_t count, float gain);

[[nodiscard]] rtc::scoped_refptr<webrtc::AudioDeviceModule> WrapAudioDeviceModule(
	rtc::scoped_refptr<webrtc::AudioDeviceModule> impl,
	std::shared_ptr<std::atomic<float>> gain);

[[nodiscard]] auto WrapAudioDeviceModuleCreator(
	std::function<rtc::scoped_refptr<webrtc::AudioDeviceModule>(webrtc::TaskQueueFactory*)> creator,
	std::shared_ptr<std::atomic<float>> gain)
-> std::function<rtc::scoped_refptr<webrtc::AudioDeviceModule>(webrtc::TaskQueueFactory*)>;

} // namespace Ayu
