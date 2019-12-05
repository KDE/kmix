//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 1996-2014 The KMix authors. Maintainer: Christian Esken <esken@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * MediaController.h
 *
 *  Created on: 17.12.2013
 *      Author: chris
 */

#ifndef MEDIACONTROLLER_H_
#define MEDIACONTROLLER_H_

#include <QString>

#include "kmixcore_export.h"

/**
 * A MediaController controls exactly one Media Player. You can think of it as a single control, like PCM.
 */
class KMIXCORE_EXPORT MediaController
{
public:
    enum PlayState { PlayPaused, PlayPlaying, PlayStopped, PlayUnknown };

	explicit MediaController(const QString &controlId);
	virtual ~MediaController() = default;

       void addMediaPlayControl() { mediaPlayControl = true; }
       void addMediaNextControl() { mediaNextControl = true; }
       void addMediaPrevControl() { mediaPrevControl = true; }
       bool hasMediaPlayControl() { return mediaPlayControl; }
       bool hasMediaNextControl() { return mediaNextControl; }
       bool hasMediaPrevControl() { return mediaPrevControl; }
	    bool hasControls() const;


	MediaController::PlayState getPlayState() const;
    void setPlayState(PlayState playState);

    bool canSkipNext();
    bool canSkipPrevious();

private:
    QString id;
    PlayState playState;

    bool mediaPlayControl;
    bool mediaNextControl;
    bool mediaPrevControl;
};

#endif /* MEDIACONTROLLER_H_ */
