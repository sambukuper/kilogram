// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ui/rp_widget.h"

namespace Ayu {

class CallVolumeSlider final : public Ui::RpWidget {
public:
	explicit CallVolumeSlider(QWidget *parent);

	void setVolume(int volume);
	[[nodiscard]] int volume() const;

	void setVolumeChangedCallback(Fn<void(int)> callback);

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseDoubleClickEvent(QMouseEvent *e) override;
	void wheelEvent(QWheelEvent *e) override;
	void enterEventHook(QEnterEvent *e) override;
	void leaveEventHook(QEvent *e) override;

private:
	void updateFromPos(const QPoint &pos);

	int _volume = 100;
	bool _dragging = false;
	bool _hovered = false;
	Fn<void(int)> _callback;
};

} // namespace Ayu
