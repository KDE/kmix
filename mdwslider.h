//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2004 Chrisitan Esken <esken@kde.org>
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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef MDWSLIDER_H
#define MDWSLIDER_H

#include <kpanelapplet.h>

#include <qwidget.h>
#include "volume.h"
#include <qptrlist.h>
#include <qpixmap.h>
#include <qrangecontrol.h>

class QBoxLayout;
class QLabel;
class QPopupMenu;
class QSlider;

class KLed;
class KLedButton;
class KAction;
class KActionCollection;
class KSmallSlider;
class KGlobalAccel;

class MixDevice;
class VerticalText;
class Mixer;
class ViewBase;

#include "mixdevicewidget.h"

class MDWSlider : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWSlider( Mixer *mixer, MixDevice* md,
	       bool showMuteLED, bool showRecordLED,
	       bool small, Qt::Orientation,
	       QWidget* parent = 0, ViewBase* mw = 0, const char* name = 0);
    ~MDWSlider();

    void addActionToPopup( KAction *action );

    bool isStereoLinked() const { return m_linked; };
    bool isLabeled() const;

    void setStereoLinked( bool value );
    void setLabeled( bool value );
    void setTicks( bool ticks );
    void setIcons( bool value );
    void setColors( QColor high, QColor low, QColor back );
    void setMutedColors( QColor high, QColor low, QColor back );
    QSize sizeHint();
    bool eventFilter( QObject* obj, QEvent* e );
    // void resizeEvent ( QResizeEvent * );
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

    bool m_linked;
    QLabel *m_iconLabel;
    KLedButton *m_muteLED;
    KLedButton *m_recordLED;
    VerticalText *m_label;
    QBoxLayout *_layout;
    QPtrList<QWidget> m_sliders;
};

#endif
