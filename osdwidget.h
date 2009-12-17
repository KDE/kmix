/*******************************************************************
* osdwidget.h
* Copyright  2009    Aurélien Gâteau <agateau@kde.org>
* Copyright  2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright  2009    Christian Esken <christian.esken@arcor.de>
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

#include <QGraphicsView>

#include <QPixmap>

class QTimer;
class QGraphicsWidget;

namespace Plasma
{
class FrameSvg;
class Label;
class Meter;
}

class OSDWidget : public QGraphicsView
{
Q_OBJECT
public:
    OSDWidget(QWidget * parent = 0);

    void setCurrentVolume(int volumeLevel, bool muted);
    void activateOSD();

    virtual QSize sizeHint() const;

protected:
    virtual void drawBackground(QPainter *painter, const QRectF &rectF);
    virtual void resizeEvent(QResizeEvent *);

private:
    Plasma::FrameSvg *m_background;
    QGraphicsScene *m_scene;
    QGraphicsWidget *m_container;
    Plasma::Label *m_iconLabel;
    Plasma::Label *m_volumeLabel;
    Plasma::Meter *m_meter;
    QTimer *m_hideTimer;

    QPixmap m_volumeHighPixmap;
    QPixmap m_volumeMediumPixmap;
    QPixmap m_volumeLowPixmap;
    QPixmap m_volumeMutedPixmap;
};

#endif
