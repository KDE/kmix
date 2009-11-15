/*******************************************************************
* osdwidget.h
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

#ifndef OSDWIDGET__H
#define OSDWIDGET__H

#include <QWidget>

#include <QPixmap>

class QProgressBar;
class QTimer;
class QLabel;

class OSDWidget : public QWidget
{
Q_OBJECT
public:
    OSDWidget(QWidget * parent = 0);
    
    void setCurrentVolume(int volumeLevel, bool muted);
    
    void showOSD();
    
private:
    QProgressBar    *m_progressBar;
    QLabel          *m_iconLabel;
    
    QTimer          *m_hideTimer;
    
    QPixmap         m_pixmapVolumeMute;
    QPixmap         m_pixmapVolumeLow;
    QPixmap         m_pixmapVolumeMed;
    QPixmap         m_pixmapVolumeHigh;
};

#endif