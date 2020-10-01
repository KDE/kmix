//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright Christian Esken <esken@kde.org>
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

#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <QSlider>
#include <QLabel>
#include <QMouseEvent>

#include "core/volume.h"


class VolumeSlider : public QSlider
{
    Q_OBJECT

public:
    VolumeSlider(Qt::Orientation orientation, QWidget *parent);
    virtual ~VolumeSlider() = default;

    Volume::ChannelID channelId() const			{ return (m_chid); }
    void setChannelId(Volume::ChannelID chid)		{ m_chid = chid; }

    QWidget *subControlLabel() const			{ return (m_subControlLabel); }
    void setSubControlLabel(QWidget *subControlLabel)	{ m_subControlLabel = subControlLabel; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateToolTip();

    Qt::Orientation m_orientation;
    QLabel *m_tooltip;

    Volume::ChannelID m_chid;
    QWidget *m_subControlLabel;
};

#endif
