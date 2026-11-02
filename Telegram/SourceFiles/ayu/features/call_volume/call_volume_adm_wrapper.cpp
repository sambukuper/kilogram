// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/call_volume/call_volume_adm_wrapper.h"

#include <api/make_ref_counted.h>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Ayu {
namespace {

class VolumeAudioTransport final : public webrtc::AudioTransport {
public:
	explicit VolumeAudioTransport(std::shared_ptr<std::atomic<float>> gain)
	: _gain(std::move(gain)) {
	}

	void setRealTransport(webrtc::AudioTransport *transport) {
		_realTransport = transport;
	}

	int32_t RecordedDataIsAvailable(
			const void *audioSamples,
			size_t nSamples,
			size_t nBytesPerSample,
			size_t nChannels,
			uint32_t samplesPerSec,
			uint32_t totalDelayMS,
			int32_t clockDrift,
			uint32_t currentMicLevel,
			bool keyPressed,
			uint32_t &newMicLevel) override {
		if (!_realTransport) {
			newMicLevel = currentMicLevel;
			return 0;
		}
		return _realTransport->RecordedDataIsAvailable(
			audioSamples,
			nSamples,
			nBytesPerSample,
			nChannels,
			samplesPerSec,
			totalDelayMS,
			clockDrift,
			currentMicLevel,
			keyPressed,
			newMicLevel);
	}

	int32_t NeedMorePlayData(
			size_t nSamples,
			size_t nBytesPerSample,
			size_t nChannels,
			uint32_t samplesPerSec,
			void *audioSamples,
			size_t &nSamplesOut,
			int64_t *elapsed_time_ms,
			int64_t *ntp_time_ms) override {
		if (!_realTransport) {
			nSamplesOut = 0;
			return 0;
		}
		const auto res = _realTransport->NeedMorePlayData(
			nSamples,
			nBytesPerSample,
			nChannels,
			samplesPerSec,
			audioSamples,
			nSamplesOut,
			elapsed_time_ms,
			ntp_time_ms);
		if (res == 0 && audioSamples && nSamplesOut > 0) {
			const auto gain = _gain ? _gain->load(std::memory_order_relaxed) : 1.0f;
			ApplyVolumeWithLimiter(
				static_cast<int16_t*>(audioSamples),
				nSamplesOut * nChannels,
				gain);
		}
		return res;
	}

	void PullRenderData(
			int bits_per_sample,
			int sample_rate,
			size_t number_of_channels,
			size_t number_of_frames,
			void *audio_data,
			int64_t *elapsed_time_ms,
			int64_t *ntp_time_ms) override {
		if (_realTransport) {
			_realTransport->PullRenderData(
				bits_per_sample,
				sample_rate,
				number_of_channels,
				number_of_frames,
				audio_data,
				elapsed_time_ms,
				ntp_time_ms);
		}
	}

private:
	std::shared_ptr<std::atomic<float>> _gain;
	webrtc::AudioTransport *_realTransport = nullptr;
};

class VolumeAudioDeviceModule final : public webrtc::AudioDeviceModule {
public:
	VolumeAudioDeviceModule(
		rtc::scoped_refptr<webrtc::AudioDeviceModule> impl,
		std::shared_ptr<std::atomic<float>> gain)
	: _impl(std::move(impl))
	, _transportWrapper(std::move(gain)) {
	}

	int32_t ActiveAudioLayer(AudioLayer *audioLayer) const override {
		return _impl->ActiveAudioLayer(audioLayer);
	}
	int32_t RegisterAudioCallback(webrtc::AudioTransport *audioCallback) override {
		_transportWrapper.setRealTransport(audioCallback);
		return _impl->RegisterAudioCallback(audioCallback ? &_transportWrapper : nullptr);
	}
	int32_t Init() override { return _impl->Init(); }
	int32_t Terminate() override { return _impl->Terminate(); }
	bool Initialized() const override { return _impl->Initialized(); }
	int16_t PlayoutDevices() override { return _impl->PlayoutDevices(); }
	int16_t RecordingDevices() override { return _impl->RecordingDevices(); }
	int32_t PlayoutDeviceName(uint16_t index, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize]) override {
		return _impl->PlayoutDeviceName(index, name, guid);
	}
	int32_t RecordingDeviceName(uint16_t index, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize]) override {
		return _impl->RecordingDeviceName(index, name, guid);
	}
	int32_t SetPlayoutDevice(uint16_t index) override { return _impl->SetPlayoutDevice(index); }
	int32_t SetPlayoutDevice(WindowsDeviceType device) override { return _impl->SetPlayoutDevice(device); }
	int32_t SetRecordingDevice(uint16_t index) override { return _impl->SetRecordingDevice(index); }
	int32_t SetRecordingDevice(WindowsDeviceType device) override { return _impl->SetRecordingDevice(device); }
	int32_t PlayoutIsAvailable(bool *available) override { return _impl->PlayoutIsAvailable(available); }
	int32_t InitPlayout() override { return _impl->InitPlayout(); }
	bool PlayoutIsInitialized() const override { return _impl->PlayoutIsInitialized(); }
	int32_t RecordingIsAvailable(bool *available) override { return _impl->RecordingIsAvailable(available); }
	int32_t InitRecording() override { return _impl->InitRecording(); }
	bool RecordingIsInitialized() const override { return _impl->RecordingIsInitialized(); }
	int32_t StartPlayout() override { return _impl->StartPlayout(); }
	int32_t StopPlayout() override { return _impl->StopPlayout(); }
	bool Playing() const override { return _impl->Playing(); }
	int32_t StartRecording() override { return _impl->StartRecording(); }
	int32_t StopRecording() override { return _impl->StopRecording(); }
	bool Recording() const override { return _impl->Recording(); }
	int32_t InitSpeaker() override { return _impl->InitSpeaker(); }
	bool SpeakerIsInitialized() const override { return _impl->SpeakerIsInitialized(); }
	int32_t InitMicrophone() override { return _impl->InitMicrophone(); }
	bool MicrophoneIsInitialized() const override { return _impl->MicrophoneIsInitialized(); }
	int32_t SpeakerVolumeIsAvailable(bool *available) override { return _impl->SpeakerVolumeIsAvailable(available); }
	int32_t SetSpeakerVolume(uint32_t volume) override { return _impl->SetSpeakerVolume(volume); }
	int32_t SpeakerVolume(uint32_t *volume) const override { return _impl->SpeakerVolume(volume); }
	int32_t MaxSpeakerVolume(uint32_t *maxVolume) const override { return _impl->MaxSpeakerVolume(maxVolume); }
	int32_t MinSpeakerVolume(uint32_t *minVolume) const override { return _impl->MinSpeakerVolume(minVolume); }
	int32_t MicrophoneVolumeIsAvailable(bool *available) override { return _impl->MicrophoneVolumeIsAvailable(available); }
	int32_t SetMicrophoneVolume(uint32_t volume) override { return _impl->SetMicrophoneVolume(volume); }
	int32_t MicrophoneVolume(uint32_t *volume) const override { return _impl->MicrophoneVolume(volume); }
	int32_t MaxMicrophoneVolume(uint32_t *maxVolume) const override { return _impl->MaxMicrophoneVolume(maxVolume); }
	int32_t MinMicrophoneVolume(uint32_t *minVolume) const override { return _impl->MinMicrophoneVolume(minVolume); }
	int32_t SpeakerMuteIsAvailable(bool *available) override { return _impl->SpeakerMuteIsAvailable(available); }
	int32_t SetSpeakerMute(bool enable) override { return _impl->SetSpeakerMute(enable); }
	int32_t SpeakerMute(bool *enabled) const override { return _impl->SpeakerMute(enabled); }
	int32_t MicrophoneMuteIsAvailable(bool *available) override { return _impl->MicrophoneMuteIsAvailable(available); }
	int32_t SetMicrophoneMute(bool enable) override { return _impl->SetMicrophoneMute(enable); }
	int32_t MicrophoneMute(bool *enabled) const override { return _impl->MicrophoneMute(enabled); }
	int32_t StereoPlayoutIsAvailable(bool *available) const override { return _impl->StereoPlayoutIsAvailable(available); }
	int32_t SetStereoPlayout(bool enable) override { return _impl->SetStereoPlayout(enable); }
	int32_t StereoPlayout(bool *enabled) const override { return _impl->StereoPlayout(enabled); }
	int32_t StereoRecordingIsAvailable(bool *available) const override { return _impl->StereoRecordingIsAvailable(available); }
	int32_t SetStereoRecording(bool enable) override { return _impl->SetStereoRecording(enable); }
	int32_t StereoRecording(bool *enabled) const override { return _impl->StereoRecording(enabled); }
	int32_t PlayoutDelay(uint16_t *delayMS) const override { return _impl->PlayoutDelay(delayMS); }
	bool BuiltInAECIsAvailable() const override { return _impl->BuiltInAECIsAvailable(); }
	bool BuiltInAGCIsAvailable() const override { return _impl->BuiltInAGCIsAvailable(); }
	bool BuiltInNSIsAvailable() const override { return _impl->BuiltInNSIsAvailable(); }
	int32_t EnableBuiltInAEC(bool enable) override { return _impl->EnableBuiltInAEC(enable); }
	int32_t EnableBuiltInAGC(bool enable) override { return _impl->EnableBuiltInAGC(enable); }
	int32_t EnableBuiltInNS(bool enable) override { return _impl->EnableBuiltInNS(enable); }

private:
	rtc::scoped_refptr<webrtc::AudioDeviceModule> _impl;
	VolumeAudioTransport _transportWrapper;
};

} // namespace

void ApplyVolumeWithLimiter(int16_t *samples, size_t count, float gain) {
	if (gain == 1.0f) {
		return;
	}
	if (gain <= 0.0f) {
		std::memset(samples, 0, count * sizeof(int16_t));
		return;
	}
	if (gain < 1.0f) {
		for (size_t i = 0; i < count; ++i) {
			samples[i] = static_cast<int16_t>(std::round(samples[i] * gain));
		}
		return;
	}

	constexpr auto kThreshold = 23166.0f;
	constexpr auto kHeadroom = 9601.0f;
	constexpr auto kInvHeadroom = 1.0f / 9601.0f;

	for (size_t i = 0; i < count; ++i) {
		const auto v = samples[i] * gain;
		const auto x = std::abs(v);
		if (x <= kThreshold) {
			samples[i] = static_cast<int16_t>(std::round(v));
		} else {
			const auto diff = x - kThreshold;
			const auto sat = kThreshold + kHeadroom * std::tanh(diff * kInvHeadroom);
			const auto res = (v < 0.0f) ? -sat : sat;
			samples[i] = static_cast<int16_t>(std::clamp(
				static_cast<int32_t>(std::round(res)),
				-32767,
				32767));
		}
	}
}

rtc::scoped_refptr<webrtc::AudioDeviceModule> WrapAudioDeviceModule(
		rtc::scoped_refptr<webrtc::AudioDeviceModule> impl,
		std::shared_ptr<std::atomic<float>> gain) {
	if (!impl) {
		return nullptr;
	}
	return rtc::make_ref_counted<VolumeAudioDeviceModule>(std::move(impl), std::move(gain));
}

auto WrapAudioDeviceModuleCreator(
	std::function<rtc::scoped_refptr<webrtc::AudioDeviceModule>(webrtc::TaskQueueFactory*)> creator,
	std::shared_ptr<std::atomic<float>> gain)
-> std::function<rtc::scoped_refptr<webrtc::AudioDeviceModule>(webrtc::TaskQueueFactory*)> {
	return [creator = std::move(creator), gain = std::move(gain)](webrtc::TaskQueueFactory *factory) {
		auto adm = creator ? creator(factory) : nullptr;
		return WrapAudioDeviceModule(std::move(adm), gain);
	};
}

} // namespace Ayu
