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

#include "KMixOSD.h"

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

KMixOSD::KMixOSD(QWidget * parent)
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
    QSize iconSize;

    if (!Plasma::Theme::defaultTheme()->imagePath("icons/audio").isEmpty()) {
        // Icons from plasma theme are 24x24 pixel
        iconSize = QSize(24, 24);
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

    //Setup the widgets
    m_background->setImagePath("widgets/tooltip");

    m_iconLabel->nativeWidget()->setPixmap(m_volumeHighPixmap);
    m_iconLabel->nativeWidget()->setFixedSize(iconSize);
    m_iconLabel->setMinimumSize(iconSize);
    m_iconLabel->setMaximumSize(iconSize);

    m_meter->setMeterType(Plasma::Meter::BarMeterHorizontal);
    m_meter->setMaximum(100);
    m_meter->setMaximumHeight(iconSize.height());

    m_volumeLabel->setAlignment(Qt::AlignCenter);
    m_volumeLabel->setMinimumHeight(iconSize.height());
    m_volumeLabel->setMaximumHeight(iconSize.height());
    m_volumeLabel->nativeWidget()->setFixedHeight(iconSize.height());
    m_volumeLabel->setWordWrap(false);

    //Set a fixed width for the volume label. To do that we need the text with the maximum width
    //(this is true if the volume is at 100%). We simply achieve that by calling "setCurrentVolume".
    setCurrentVolume(100, false);
    themeUpdated();

    //Setup the auto-hide timer
    m_hideTimer->setInterval(2000);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, SIGNAL(timeout()), this, SLOT(hide()));

    //Setup the OSD layout
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(m_container);
    layout->addItem(m_iconLabel);
    layout->addItem(m_meter);
    layout->addItem(m_volumeLabel);

    m_scene->addItem(m_container);

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeUpdated())); // e.g. for updating font

    setScene(m_scene);
}

void KMixOSD::activateOSD()
{
    m_hideTimer->start();
}

void KMixOSD::themeUpdated()
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
    m_volumeLabel->setMaximumWidth(widthHint);
    m_volumeLabel->nativeWidget()->setFixedWidth(widthHint);
//    m_volumeLabel->setText(oldText);
}


/**
 * Set volume level in percent
 */
void KMixOSD::setCurrentVolume(int volumeLevel, bool muted)
{
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

void KMixOSD::drawBackground(QPainter *painter, const QRectF &/*rectF*/)
{
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    m_background->paintFrame(painter);
    painter->restore();
}

QSize KMixOSD::sizeHint() const
{
    int iconSize = m_iconLabel->nativeWidget()->pixmap()->height();
    int labelWidth = m_volumeLabel->nativeWidget()->size().width();
    int meterHeight = iconSize;
    int meterWidth = iconSize * 12;
    qreal left, top, right, bottom;
    m_background->getMargins(left, top, right, bottom);
    return QSize(meterWidth + labelWidth + iconSize + left + right, meterHeight + top + bottom);
}

void KMixOSD::resizeEvent(QResizeEvent*)
{
    m_background->resizeFrame(size());
    m_container->setGeometry(0, 0, width(), height());
    qreal left, top, right, bottom;
    m_background->getMargins(left, top, right, bottom);
    m_container->layout()->setContentsMargins(left, top, right, bottom);

    m_scene->setSceneRect(0, 0, width(), height());
    if (!KWindowSystem::compositingActive()) {
        setMask(m_background->mask());
    }
}

void KMixOSD::showEvent(QShowEvent *)
{
    Plasma::WindowEffects::overrideShadow(winId(), true);
    Plasma::WindowEffects::enableBlurBehind(winId(), true, m_background->mask());
}

#include "KMixOSD.moc"