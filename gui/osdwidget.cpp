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

#include "gui/osdwidget.h"

// Qt
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QTimer>
#include <QLabel>

// KDE
#include <KIcon>
#include <KDialog>
#include <KWindowSystem>
#include <Plasma/FrameSvg>
#include <Plasma/Label>
#include <Plasma/Meter>
#include <Plasma/Theme>
#include <Plasma/WindowEffects>

#include "core/ControlManager.h"
#include <core/mixer.h>

OSDWidget::OSDWidget(QWidget * parent)
    : Plasma::Dialog(parent, Qt::ToolTip),
    m_scene(new QGraphicsScene(this)),
    m_container(new QGraphicsWidget),
    m_iconLabel(new Plasma::Label),
    m_volumeLabel(new Plasma::Label),
    m_meter(new Plasma::Meter),
    m_hideTimer(new QTimer(this))
{
    //Setup the window properties
    KWindowSystem::setState(winId(), NET::KeepAbove);
    KWindowSystem::setType(winId(), NET::Tooltip);
    setAttribute(Qt::WA_X11NetWmWindowTypeToolTip, true);

    m_meter->setMeterType(Plasma::Meter::BarMeterHorizontal);
    m_meter->setMaximum(100);

    //Set a fixed width for the volume label. To do that we need the text with the maximum width
    //(this is true if the volume is at 100%). We simply achieve that by calling "setCurrentVolume".
    setCurrentVolume(100, false);

    /* We are registering for volume changes of all cards. An alternative
     * would be to register to volume changes of the global master and additionally
     * register to MasterChanges. That could be slightly more efficient
     */
    ControlManager::instance().addListener(
        QString(), // all mixers
        ControlChangeType::Volume,
        this,
        QString("OSDWidget")
        );

    //Setup the auto-hide timer
    m_hideTimer->setInterval(2000);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, SIGNAL(timeout()), this, SLOT(hide()));

    //Setup the OSD layout
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(m_container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addItem(m_iconLabel);
    layout->addItem(m_meter);
    layout->addItem(m_volumeLabel);

    m_scene->addItem(m_container);
    setGraphicsWidget(m_container);

    themeUpdated();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeUpdated())); // e.g. for updating font
}


OSDWidget::~OSDWidget()
{
  ControlManager::instance().removeListener(this);
}
void OSDWidget::controlsChange(int changeType)
{
  ControlChangeType::Type type = ControlChangeType::fromInt(changeType);
    shared_ptr<MixDevice> master = Mixer::getGlobalMasterMD();
  switch (type )
  {
    case ControlChangeType::Volume:
      if ( master )
      {
	setCurrentVolume(master->playbackVolume().getAvgVolumePercent(Volume::MALL), master->isMuted());
      }
      break;

    default:
      ControlManager::warnUnexpectedChangeType(type, this);
      break;
  }

}
void OSDWidget::activateOSD()
{
    m_hideTimer->start();
}

void OSDWidget::themeUpdated()
{
    //Set a font which makes the text appear as big (height-wise) as the meter.
    //QFont font = QFont(m_volumeLabel->nativeWidget()->font());
    Plasma::Theme* theme = Plasma::Theme::defaultTheme();


    QPalette palette = m_volumeLabel->palette();
    palette.setColor(QPalette::WindowText, theme->color(Plasma::Theme::TextColor));
    m_volumeLabel->setPalette(palette);

    QFont font = theme->font(Plasma::Theme::DefaultFont);
    font.setPointSize(15);
    m_volumeLabel->setFont(font);
    QFontMetrics qfm(font);
    QRect textSize = qfm.boundingRect("100 %  ");

    int widthHint = textSize.width();
    int heightHint = textSize.height();
    //setCurrentVolume(100,false);
    m_volumeLabel->setMinimumWidth(widthHint);
    m_volumeLabel->setMaximumHeight(heightHint);
    m_volumeLabel->nativeWidget()->setFixedWidth(widthHint);
//    m_volumeLabel->setText(oldText);

    //Cache the icon pixmaps
    QSize iconSize;

    if (!Plasma::Theme::defaultTheme()->imagePath("icons/audio").isEmpty()) {
        QFontMetrics fm(m_volumeLabel->font());
        iconSize = QSize(fm.height(), fm.height());
        // Leak | low prio | The old Plasma::Svg is not freed on a themeUpdated(), also it is not freed in the destructor
        Plasma::Svg *svgIcon = new Plasma::Svg(this);
        svgIcon->setImagePath("icons/audio");
        svgIcon->setContainsMultipleImages(true);
        svgIcon->resize(iconSize);
        m_volumeHighPixmap = svgIcon->pixmap("audio-volume-high");
        m_volumeMediumPixmap = svgIcon->pixmap("audio-volume-medium");
        m_volumeLowPixmap = svgIcon->pixmap("audio-volume-low");
        m_volumeMutedPixmap = svgIcon->pixmap("audio-volume-muted");
    } else {
        iconSize = QSize(KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium);
        m_volumeHighPixmap = KIcon( QLatin1String( "audio-volume-high" )).pixmap(iconSize);
        m_volumeMediumPixmap = KIcon( QLatin1String( "audio-volume-medium" )).pixmap(iconSize);
        m_volumeLowPixmap = KIcon( QLatin1String( "audio-volume-low" )).pixmap(iconSize);
        m_volumeMutedPixmap = KIcon( QLatin1String( "audio-volume-muted" )).pixmap(iconSize);
    }

    m_iconLabel->nativeWidget()->setPixmap(m_volumeHighPixmap);
    m_iconLabel->nativeWidget()->setFixedSize(iconSize);
    m_iconLabel->setMinimumSize(iconSize);
    m_iconLabel->setMaximumSize(iconSize);

    m_meter->setMaximumHeight(iconSize.height());

    m_volumeLabel->setMinimumHeight(iconSize.height());
    m_volumeLabel->setMaximumHeight(iconSize.height());
    m_volumeLabel->nativeWidget()->setFixedHeight(iconSize.height());

    m_volumeLabel->setAlignment(Qt::AlignCenter);
    m_volumeLabel->setWordWrap(false);

    m_container->setMinimumSize(iconSize.width() * 13 + m_volumeLabel->nativeWidget()->width(), iconSize.height());
    m_container->setMaximumSize(iconSize.width() * 13 + m_volumeLabel->nativeWidget()->width(), iconSize.height());

    syncToGraphicsWidget();
}


/**
 * Set volume level in percent
 */
void OSDWidget::setCurrentVolume(int volumeLevel, bool muted)
{
    kDebug() << "Meter is visible: " << m_meter->isVisible();

    if ( muted )
    {
        volumeLevel = 0;
    }
    m_meter->setValue(volumeLevel);

    if (!muted && (volumeLevel > 0)) {
        if (volumeLevel < 25) {
            m_iconLabel->nativeWidget()->setPixmap(m_volumeLowPixmap);
        } else if (volumeLevel < 75) {
            m_iconLabel->nativeWidget()->setPixmap(m_volumeMediumPixmap);
        } else {
            m_iconLabel->nativeWidget()->setPixmap(m_volumeHighPixmap);
        }
    } else {
        m_iconLabel->nativeWidget()->setPixmap(m_volumeMutedPixmap);
    }

    //Show the volume %
    m_volumeLabel->setText(QString::number(volumeLevel) + " %"); // if you change the text, please adjust textSize in themeUpdated()
}

#include "osdwidget.moc"
