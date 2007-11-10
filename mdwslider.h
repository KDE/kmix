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

#include <QCheckBox>
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
    MDWSlider( MixDevice* md,
	       bool showMuteLED, bool showRecordLED,
	       bool small, Qt::Orientation,
	       QWidget* parent = 0, ViewBase* mw = 0);
    ~MDWSlider() {}

    void addActionToPopup( KAction *action );

    // GUI
    bool isStereoLinked() const { return m_linked; }
    void setStereoLinked( bool value );
    void setLabeled( bool value );
    void setTicks( bool ticks );
    void setIcons( bool value );
    void setColors( QColor high, QColor low, QColor back );
    void setMutedColors( QColor high, QColor low, QColor back );
    
    bool eventFilter( QObject* obj, QEvent* e );
    
    // Layout
    QSizePolicy sizePolicy() const;
    int playbackExtentHint() const;
    void setPlaybackExtent(int extent);

    
public slots:
    void toggleRecsrc();
    void toggleMuted();
    void toggleStereoLinked();

    void setDisabled();
    void setDisabled( bool value );
    void update();
    virtual void showContextMenu();


signals:
//    void newVolume( int num, Volume volume );
//    void newMasterVolume( Volume volume );
//    void masterMuted( bool );
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
    QPixmap loadIcon( const char* filename );
    void createWidgets( bool showMuteLED, bool showCaptureLED );
    void createWidgetsTopPart(QBoxLayout *, bool showMuteLED);
    void createWidgetsBottomPart(QBoxLayout *, bool showCaptureLED);
    void addSliders( QBoxLayout *volLayout, char type, bool addLabel);

    // Methods that are called two times from a wrapper. Once for playabck, once for capture
    void setStereoLinkedInternal( QList<QWidget *>& ref_sliders );
    void setTicksInternal( QList<QWidget *>& ref_sliders,  bool ticks );
    void volumeChangeInternal(Volume& vol, QList<Volume::ChannelID>& slidersChids, QList<QWidget *>& ref_sliders );
    void updateInternal(Volume& vol, QList<QWidget *>& ref_sliders, QList<Volume::ChannelID>& slidersChids);


    bool m_linked;
    
    // GUI: Top portion ( Icon + Mute)
    QLabel      *m_iconLabelSimple;
    QCheckBox* m_qcb;
    QLabel* m_muteText;
    QBoxLayout *m_iconLayout;
    QWidget *m_playbackSpacer;
        
    // GUI: Middle portion (Sliders , including labels)
    QBoxLayout *_layout;
    QWidget *m_extraCaptureLabel; // extra capture label (if you got playback AND capture on the same control)
    //KLedButton *m_recordLED;
    QWidget *m_label; // is either QLabel or VerticalText
    
     // GUI: Lower portion (Capture)
    QCheckBox* m_captureLED;
    QLabel* m_captureText;
    QWidget *m_captureSpacer;


    QList<QWidget *> m_slidersPlayback;
    QList<QWidget *> m_slidersCapture;
    QList<Volume::ChannelID> _slidersChidsPlayback;
    QList<Volume::ChannelID> _slidersChidsCapture;

};

#endif
