/*******************************************************************
* osdwidget.cpp
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

#include "osdwidget.h"

// Qt
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QTimer>
#include <QLabel>

// KDE
#include <KIcon>
#include <KDialog>
#include <Plasma/FrameSvg>
#include <Plasma/Label>
#include <Plasma/Meter>

OSDWidget::OSDWidget(QWidget * parent)
    : QGraphicsView(parent),
    m_background(new Plasma::FrameSvg(this)),
    m_scene(new QGraphicsScene(this)),
    m_container(new QGraphicsWidget),
    m_iconLabel(new Plasma::Label),
    m_volumeLabel(new Plasma::Label),
    m_meter(new Plasma::Meter),
    m_hideTimer(new QTimer(this))
{
    //Setup the window properties
    setWindowFlags(Qt::X11BypassWindowManagerHint);
    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAttribute(Qt::WA_TranslucentBackground);

    //Cache the icon pixmaps
    QSize iconSize = QSize(25, 25);
    volumeHighIconPixmap = KIcon("audio-volume-high").pixmap(iconSize);
    volumeMediumIconPixmap = KIcon("audio-volume-medium").pixmap(iconSize);
    volumeLowIconPixmap = KIcon("audio-volume-low").pixmap(iconSize);
    volumeMutedIconPixmap = KIcon("audio-volume-muted").pixmap(iconSize);

    //Setup the widgets
    m_background->setImagePath("widgets/tooltip");

    m_iconLabel->nativeWidget()->setPixmap(volumeHighIconPixmap);
    m_iconLabel->nativeWidget()->setFixedSize(iconSize);

    m_meter->setMeterType(Plasma::Meter::BarMeterHorizontal);
    m_meter->setMaximum(100);

    m_volumeLabel->setAlignment(Qt::AlignCenter);

    //Setup the auto-hide timer
    m_hideTimer->setInterval(2000);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, SIGNAL(timeout()), this, SLOT(hide()));

    //Setup the OSD layout
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(m_container);
    layout->addItem(m_iconLabel);
    layout->addItem(m_meter);

    m_scene->addItem(m_container);
    setScene(m_scene);
}

void OSDWidget::activateOSD()
{
    m_hideTimer->start();
}

void OSDWidget::setCurrentVolume(int volumeLevel, bool muted)
{
    m_meter->setValue(volumeLevel);

    if (!muted) {
        if (volumeLevel < 25) {
            m_iconLabel->nativeWidget()->setPixmap(volumeLowIconPixmap);
        } else if (volumeLevel < 75) {
            m_iconLabel->nativeWidget()->setPixmap(volumeMediumIconPixmap);
        } else {
            m_iconLabel->nativeWidget()->setPixmap(volumeHighIconPixmap);
        }
    } else {
        m_iconLabel->nativeWidget()->setPixmap(volumeMutedIconPixmap);
    }

    //Show the volume %
    //m_meter->setLabel(0, QString::number(volumeLevel) + " %");
}

void OSDWidget::drawBackground(QPainter *painter, const QRectF &/*rectF*/)
{
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    m_background->paintFrame(painter);
    painter->restore();
}

QSize OSDWidget::sizeHint() const
{
    int iconSize = m_iconLabel->nativeWidget()->pixmap()->height();
    int meterHeight = iconSize;
    int meterWidth = iconSize * 12;
    qreal left, top, right, bottom;
    m_background->getMargins(left, top, right, bottom);
    return QSize(meterWidth + iconSize + left + right, meterHeight + top + bottom);
}

void OSDWidget::resizeEvent(QResizeEvent*)
{
    m_background->resizeFrame(size());
    m_container->setGeometry(0, 0, width(), height());
    m_meter->setMaximumHeight(m_iconLabel->geometry().height());
    qreal left, top, right, bottom;
    m_background->getMargins(left, top, right, bottom);
    m_container->layout()->setContentsMargins(left, top, right, bottom);

    m_scene->setSceneRect(0, 0, width(), height());
    setMask(m_background->mask());
}

#include "osdwidget.moc"
