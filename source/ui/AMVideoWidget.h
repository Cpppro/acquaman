/*
Copyright 2010, 2011 Mark Boots, David Chevrier, and Darren Hunter.

This file is part of the Acquaman Data Acquisition and Management framework ("Acquaman").

Acquaman is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Acquaman is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Acquaman.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef AMVIDEOWIDGET_H
#define AMVIDEOWIDGET_H

#include <QFrame>
#include "vlc/libvlc.h"
#include "vlc/libvlc_media.h"
#include "vlc/libvlc_media_player.h"

class QMacCocoaViewContainer;

class AMVideoWidget : public QFrame
{
	Q_OBJECT
public:
	explicit AMVideoWidget(QWidget *parent = 0);
	virtual ~AMVideoWidget();

	/// Open a video file or network stream. Returns true on success
	/*! Formats:
	[file://]filename              Plain media file
	http://ip:port/file            HTTP URL
	ftp://ip:port/file             FTP URL
	mms://ip:port/file             MMS URL
	screen://                      Screen capture
	[dvd://][device][@raw_device]  DVD device
	[vcd://][device]               VCD device
	[cdda://][device]              Audio CD device
	udp:[[<source address>]@[<bind address>][:<bind port>]]
	*/
	bool openVideoUrl(const QString& videoUrl);

	/// isPlaying is true when video is in playback
	bool isPlaying() const { return libvlc_media_player_is_playing(vlcPlayer_); }
	/// returns the exact state of playback (see libvlc_state_t)
	libvlc_state_t state() const { return libvlc_media_player_get_state(vlcPlayer_); }

signals:

public slots:

	/// Start playback
	void play() { libvlc_media_player_play(vlcPlayer_); }

	/// Pause playback
	void pause() { libvlc_media_player_set_pause(vlcPlayer_, true); }
	/// Resume playback
	void unPause() { libvlc_media_player_set_pause(vlcPlayer_, false); }
	/// Convenience function to toggle pause on/off
	void togglePause() { libvlc_media_player_pause(vlcPlayer_); }

	/// Stop playback and rewind to the beginning
	void stop() { libvlc_media_player_stop(vlcPlayer_); }

protected:

	libvlc_instance_t* vlcInstance_;
	libvlc_media_player_t* vlcPlayer_;

#ifdef Q_WS_MAC
	QMacCocoaViewContainer* macViewContainer_;
#endif

};

#endif // AMVIDEOWIDGET_H
