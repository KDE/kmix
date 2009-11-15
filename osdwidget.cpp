/*******************************************************************
* osdwidget.cpp
* Copyright  2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#include "osdwidget.h"

#include <QDesktopWidget>
#include <QCursor>
#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>

#include <QTimer>

#include <KApplication>
#include <KDialog>
#include <KIcon>

OSDWidget::OSDWidget(QWidget * parent)
    : QWidget(parent)
{
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(64,64);
    
    QHBoxLayout * layout = new QHBoxLayout();
    layout->setSpacing(5);
    layout->setContentsMargins(5,5,5,5);
    
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_progressBar);
    
    setLayout(layout);
    
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, SIGNAL(timeout()), this, SLOT(hide()));
    
    setWindowFlags(Qt::X11BypassWindowManagerHint);
    
    m_pixmapVolumeMute = KIcon("audio-volume-muted").pixmap(64);
    m_pixmapVolumeLow = KIcon("audio-volume-low").pixmap(64);
    m_pixmapVolumeMed = KIcon("audio-volume-medium").pixmap(64);
    m_pixmapVolumeHigh = KIcon("audio-volume-high").pixmap(64);
}

void OSDWidget::showOSD()
{
    show();
    KDialog::centerOnScreen(this);
    m_hideTimer->start(2000);
}

void OSDWidget::setCurrentVolume(int volumeLevel, bool muted)
{
    m_progressBar->setValue(volumeLevel);
    
    if (!muted) {
        if (volumeLevel < 25) {
            m_iconLabel->setPixmap(m_pixmapVolumeLow);
        } else if (volumeLevel < 75) {
            m_iconLabel->setPixmap(m_pixmapVolumeMed);
        } else {
            m_iconLabel->setPixmap(m_pixmapVolumeHigh);
        }
    } else {
        m_iconLabel->setPixmap(m_pixmapVolumeMute);
    }
}
