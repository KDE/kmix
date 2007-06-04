//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright Chrisitan Esken <esken@kde.org>
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

#ifndef MDWSLIDER_H
#define MDWSLIDER_H

#include <QList>
#include <QWidget>
#include <qlist.h>
#include <qpixmap.h>

class QBoxLayout;
class QToolButton;
class QLabel;

class KLed;
class KLedButton;
class KAction;

class MixDevice;
class VerticalText;
class Mixer;
class ViewBase;

#include "mixdevicewidget.h"
#include "volume.h"


class MDWSlider : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWSlider( Mixer *mixer, MixDevice* md,
	       bool showMuteLED, bool showRecordLED,
	       bool small, Qt::Orientation,
	       QWidget* parent = 0, ViewBase* mw = 0);
    ~MDWSlider() {}

    void addActionToPopup( KAction *action );

    bool isStereoLinked() const { return m_linked; }
    bool isLabeled() const;

    void setStereoLinked( bool value );
    void setLabeled( bool value );
    void setTicks( bool ticks );
    void setIcons( bool value );
    void setColors( QColor high, QColor low, QColor back );
    void setMutedColors( QColor high, QColor low, QColor back );
    QSize sizeHint() const;
    bool eventFilter( QObject* obj, QEvent* e );
    QSizePolicy sizePolicy() const;

public slots:
    void toggleRecsrc();
    void toggleMuted();
    void toggleStereoLinked();

    void setDisabled();
    void setDisabled( bool value );
    void update();
    virtual void showContextMenu();


signals:
    void newVolume( int num, Volume volume );
    void newMasterVolume( Volume volume );
    void masterMuted( bool );
    void newRecsrc( int num, bool on );
    void toggleMenuBar(bool value);

private slots:
    void setRecsrc( bool value );
    void setMuted(bool value);
    void volumeChange( int );

    void increaseVolume();
    void decreaseVolume();

private:
    QPixmap icon( int icontype );
    void setIcon( int icontype );
    void createWidgets( bool showMuteLED, bool showRecordLED );
    void addSliders( QBoxLayout *volLayout, Volume& vol, const char* debug_text);

    bool m_linked;
    QToolButton *m_iconLabel;
    KLedButton *m_recordLED;
    QWidget *m_label; // is either QLabel or VerticalText
    QBoxLayout *_layout;
    QList<QWidget *> m_sliders;
    QList<Volume::ChannelID> _slidersChids;
};

#endif
