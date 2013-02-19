/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#ifndef CONTROLSLIDER_H
#define CONTROLSLIDER_H

#include <QtGui/QWidget>

class OrgKdeKMixControlInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef OrgKdeKMixControlInterface Control;
        }
    }
}

class QSlider;
class QPushButton;
class QComboBox;

class ControlSlider : public QWidget
{
    Q_OBJECT

public:
    explicit ControlSlider(org::kde::KMix::Control *control, QWidget *parent = 0);
    ~ControlSlider();
    QSize minimumSizeHint() const;

private slots:
    void volumeChange(int channel, int level);
    void updateVolume(int channel);
    void updateMute();
    void toggleMute();
    void changeTarget(int idx);
    void handleTargetChange();
private:
    QPushButton *m_mute;
    org::kde::KMix::Control *m_control;
    QList<QSlider*> m_sliders;
    QComboBox *m_targetSwitcher;
    QPushButton *m_channelLock;
};

#endif // CONTROLSLIDER_H
