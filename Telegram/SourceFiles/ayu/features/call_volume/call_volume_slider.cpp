// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/call_volume/call_volume_slider.h"

#include "ui/painter.h"
#include "styles/style_basic.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <algorithm>
#include <cmath>

namespace Ayu {

CallVolumeSlider::CallVolumeSlider(QWidget *parent)
: Ui::RpWidget(parent) {
	setMouseTracking(true);
}

void CallVolumeSlider::setVolume(int volume) {
	const auto clamped = std::clamp(volume, 0, 200);
	if (_volume == clamped) {
		return;
	}
	_volume = clamped;
	update();
}

int CallVolumeSlider::volume() const {
	return _volume;
}

void CallVolumeSlider::setVolumeChangedCallback(Fn<void(int)> callback) {
	_callback = std::move(callback);
}

void CallVolumeSlider::updateFromPos(const QPoint &pos) {
	const auto trackTop = 26.0f;
	const auto trackBottom = height() - 26.0f;
	const auto trackHeight = trackBottom - trackTop;
	if (trackHeight <= 0.0f) {
		return;
	}

	const auto ratio = std::clamp((trackBottom - pos.y()) / trackHeight, 0.0f, 1.0f);
	auto newVol = static_cast<int>(std::round(ratio * 200.0f));

	if (newVol >= 97 && newVol <= 103) {
		newVol = 100;
	}
	newVol = std::clamp(newVol, 0, 200);

	if (_volume != newVol) {
		_volume = newVol;
		update();
		if (_callback) {
			_callback(_volume);
		}
	}
}

void CallVolumeSlider::paintEvent(QPaintEvent *e) {
	Painter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const auto w = width();
	const auto h = height();
	const auto trackWidth = 14.0f;
	const auto trackLeft = (w - trackWidth) / 2.0f;
	const auto trackTop = 26.0f;
	const auto trackBottom = h - 26.0f;
	const auto trackHeight = trackBottom - trackTop;
	if (trackHeight <= 0.0f) {
		return;
	}

	const auto trackRect = QRectF(trackLeft, trackTop, trackWidth, trackHeight);
	const auto cornerRadius = trackWidth / 2.0f;

	// Background track pill
	p.setPen(Qt::NoPen);
	p.setBrush(QColor(0, 0, 0, _hovered ? 140 : 100));
	p.drawRoundedRect(trackRect, cornerRadius, cornerRadius);

	// 100% center notch line
	const auto midY = trackTop + trackHeight / 2.0f;
	p.setPen(QPen(QColor(255, 255, 255, 130), 1.5f));
	p.drawLine(QPointF(trackLeft - 2.0f, midY), QPointF(trackLeft + trackWidth + 2.0f, midY));

	// Fill progress
	const auto fillRatio = std::clamp(_volume / 200.0f, 0.0f, 1.0f);
	const auto fillHeight = trackHeight * fillRatio;
	const auto fillTop = trackBottom - fillHeight;

	if (_volume > 0) {
		QPainterPath clipPath;
		clipPath.addRoundedRect(trackRect, cornerRadius, cornerRadius);
		p.save();
		p.setClipPath(clipPath);

		if (_volume <= 100) {
			p.fillRect(QRectF(trackLeft, fillTop, trackWidth, fillHeight), QColor(74, 222, 128));
		} else {
			// Normal range in green
			p.fillRect(QRectF(trackLeft, midY, trackWidth, trackBottom - midY), QColor(74, 222, 128));
			// Boost range in amber/orange
			p.fillRect(QRectF(trackLeft, fillTop, trackWidth, midY - fillTop), QColor(245, 166, 35));
		}
		p.restore();
	}

	// Thumb circle
	const auto thumbRadius = 7.5f;
	p.setPen(QPen(QColor(0, 0, 0, 50), 1.0f));
	p.setBrush(Qt::white);
	p.drawEllipse(QPointF(w / 2.0f, fillTop), thumbRadius, thumbRadius);

	// Top percentage label
	p.setFont(st::semiboldFont);
	p.setPen(Qt::white);
	const auto label = (_volume == 0)
		? u"Mute"_q
		: (QString::number(_volume) + u"%"_q);
	p.drawText(QRect(0, 2, w, 22), Qt::AlignCenter, label);

	// Bottom icon/label
	const auto icon = (_volume == 0)
		? QString::fromUtf8("🔇")
		: (_volume <= 100 ? QString::fromUtf8("🔉") : QString::fromUtf8("🔊"));
	p.drawText(QRect(0, h - 24, w, 22), Qt::AlignCenter, icon);
}

void CallVolumeSlider::mousePressEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton) {
		_dragging = true;
		updateFromPos(e->pos());
	}
}

void CallVolumeSlider::mouseMoveEvent(QMouseEvent *e) {
	if (_dragging) {
		updateFromPos(e->pos());
	}
}

void CallVolumeSlider::mouseReleaseEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton) {
		_dragging = false;
	}
}

void CallVolumeSlider::mouseDoubleClickEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton) {
		setVolume(100);
		if (_callback) {
			_callback(100);
		}
	}
}

void CallVolumeSlider::wheelEvent(QWheelEvent *e) {
	const auto numDegrees = e->angleDelta().y();
	if (numDegrees == 0) {
		return;
	}
	const auto step = (numDegrees > 0) ? 5 : -5;
	auto newVol = _volume + step;
	if (newVol >= 96 && newVol <= 104) {
		newVol = 100;
	}
	newVol = std::clamp(newVol, 0, 200);
	if (_volume != newVol) {
		_volume = newVol;
		update();
		if (_callback) {
			_callback(_volume);
		}
	}
}

void CallVolumeSlider::enterEventHook(QEnterEvent *e) {
	_hovered = true;
	update();
}

void CallVolumeSlider::leaveEventHook(QEvent *e) {
	_hovered = false;
	update();
}

} // namespace Ayu
