/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) Trever Fischer <tdfischer@fedoraproject.org>
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

#include "ControlSlider.h"
#include "ControlMonitor.h"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtCore/QSignalMapper>
#include <KDE/KIcon>
#include <QtGui/QPushButton>
#include <QtGui/QProgressBar>
#include <QtGui/QComboBox>

#include "control_interface.h"

const QString KMIX_DBUS_SERVICE = "org.kde.kmixd";

ControlSlider::ControlSlider(org::kde::KMix::Control *control, QWidget *parent)
    : QWidget(parent)
    , m_mute(0)
    , m_control(control)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);

    QWidget *labelContainer = new QWidget(this);
    QHBoxLayout *labelLayout = new QHBoxLayout(labelContainer);
    labelContainer->setLayout(labelLayout);

    QLabel *label = new QLabel(m_control->displayName(), labelContainer);
    QLabel *iconLabel = new QLabel(labelContainer);
    KIcon icon(control->iconName());
    iconLabel->setPixmap(icon.pixmap(QSize(32, 32)));

    m_channelLock = new QPushButton(labelContainer);
    m_channelLock->setIcon(KIcon("object-locked"));
    m_channelLock->setCheckable(true);
    m_channelLock->setChecked(true);

    labelLayout->addWidget(iconLabel);
    labelLayout->addWidget(label);
    labelLayout->addWidget(m_channelLock);

    QWidget *sliderContainer = new QWidget(this);
    QVBoxLayout *sliderLayout = new QVBoxLayout(sliderContainer);

    QSignalMapper *mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(updateVolume(int)));
    for(int i = 0;i<control->channels();i++) {
        QSlider *slider = new QSlider(sliderContainer);
        sliderLayout->addWidget(slider);
        slider->setMaximum(65536);
        slider->setValue(control->getVolume(i));
        slider->setOrientation(Qt::Horizontal);
        mapper->setMapping(slider, i);
        m_sliders << slider;
        connect(slider, SIGNAL(valueChanged(int)), mapper, SLOT(map()));
    }

    layout->addWidget(labelContainer);
    layout->addWidget(sliderContainer);
    if (control->canMonitor()) {
        QProgressBar *levelDisplay = new QProgressBar(this);
        levelDisplay->setMaximum(65536);
        new ControlMonitor(levelDisplay, control, levelDisplay);
        layout->addWidget(levelDisplay);
    }
    if (control->canMute()) {
        m_mute = new QPushButton(this);
        connect(m_mute, SIGNAL(clicked(bool)), this, SLOT(toggleMute()));
        layout->addWidget(m_mute);
    }

    if (control->canChangeTarget()) {
        m_targetSwitcher = new QComboBox(this);
        int i = 0;
        int currentIdx = 0;
        foreach(const QString &path, control->alternateTargets()) {
            org::kde::KMix::Control target(KMIX_DBUS_SERVICE, path, QDBusConnection::sessionBus());
            m_targetSwitcher->insertItem(i, target.displayName(), target.id());
            if (control->currentTarget() == path)
                currentIdx = i;
            i++;
        }
        m_targetSwitcher->setCurrentIndex(currentIdx);
        connect(m_targetSwitcher, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTarget(int)));
        layout->addWidget(m_targetSwitcher);
    }

    updateMute();
    connect(control, SIGNAL(volumeChanged(int, int)), this, SLOT(volumeChange(int, int)));
    connect(control, SIGNAL(muteChanged(bool)), this, SLOT(updateMute()));
    connect(control, SIGNAL(currentTargetChanged(QString)), this, SLOT(handleTargetChange()));

}

ControlSlider::~ControlSlider()
{
}

void ControlSlider::volumeChange(int channel, int level)
{
    bool oldstate = m_sliders[channel]->blockSignals(true);
    m_sliders[channel]->setValue(level);
    m_sliders[channel]->blockSignals(oldstate);
}

void ControlSlider::updateVolume(int channel)
{
    int value = m_sliders[channel]->value();
    if (m_channelLock->isChecked()) {
        for(int i = 0;i<m_control->channels();i++) {
            m_control->setVolume(i, value);
            if (i != channel)
                m_sliders[i]->setValue(value);
        }
    } else {
        m_control->setVolume(channel, value);
    }
}

void ControlSlider::updateMute()
{
    if (m_mute) {
        KIcon icon;
        if (m_control->mute()) {
            icon = KIcon("audio-volume-muted");
        } else {
            icon = KIcon("audio-volume-high");
        }
        m_mute->setIcon(icon);
    }
}

void ControlSlider::toggleMute()
{
    if (m_mute) {
        m_control->setMute(!m_control->mute());
    }
}

void ControlSlider::changeTarget(int idx)
{
    if (m_targetSwitcher) {
        m_control->setTarget(m_targetSwitcher->itemData(idx).toInt());
    }
}

void ControlSlider::handleTargetChange()
{
    if (m_targetSwitcher) {
        QString path = m_control->currentTarget();
        org::kde::KMix::Control target(KMIX_DBUS_SERVICE, path, QDBusConnection::sessionBus());
        int current = target.id();
        qDebug() << "Target changed to" << current;
        for(int i = 0;i<m_targetSwitcher->count();i++) {
            if (m_targetSwitcher->itemData(i).toInt() == current) {
                m_targetSwitcher->setCurrentIndex(i);
                return;
            }
        }
    }
}

QSize ControlSlider::minimumSizeHint() const
{
    QSize orig = QWidget::minimumSizeHint();
    return QSize(400, orig.height());
}


#include "ControlSlider.moc"
