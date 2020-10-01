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

#include <qlist.h>

#include "gui/volumeslider.h"
#include "gui/mixdevicewidget.h"
#include "core/volume.h"


class QBoxLayout;
class QGridLayout;
class QToolButton;
class QLabel;
class QMenu;

class MixDevice;
class VerticalText;
class ViewBase;
class ToggleToolButton;


class MDWSlider : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWSlider(shared_ptr<MixDevice> md, MixDeviceWidget::MDWFlags flags, ViewBase *view, ProfControl *pctl = nullptr);
    virtual ~MDWSlider();

    // GUI
    bool isStereoLinked() const override		{ return (m_linked); }
    void setStereoLinked(bool value) override;

    void setLabeled(bool value) override;
    void setTicks(bool ticks) override;
    void setIcons(bool value) override;

    QToolButton* addMediaButton(QString iconName, QLayout* layout, QWidget *parent);
    void updateMediaButton();
    
    QString iconName();
    // Layout
    QSizePolicy sizePolicy() const;
	QSize sizeHint() const override;
    int labelExtentHint() const override;
    void setLabelExtent(int extent) override;

public slots:
    void toggleRecsrc();
    void toggleMuted();
    void toggleStereoLinked();

    void update() override;
    void increaseOrDecreaseVolume(bool arg1, Volume::VolumeTypeFlag volumeType);
    void addMediaControls(QBoxLayout* arg1);

protected:
    void createContextMenu(QMenu *menu) override;

    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void setRecsrc(bool value);
    void setMuted(bool value);
    void volumeChange( int );
    void sliderPressed();
    void sliderReleased();

    void increaseVolume();
    void decreaseVolume();

    void mediaPlay(bool);
    void mediaNext(bool);
    void mediaPrev(bool);

    void showMoveMenu();
    void moveStream(bool checked);

private:
    void createWidgets();
    void createActions();
    void createGlobalActions();

    void addSliders(QBoxLayout *volLayout, char type, Volume& vol,
                     QList<QAbstractSlider *>& ref_sliders, const QString &tooltipText );

    // Methods that are called two times from a wrapper. Once for playabck, once for capture
    void setStereoLinkedInternal( QList< QAbstractSlider* >& ref_sliders, bool showSubcontrolLabels);
    void setTicksInternal( QList< QAbstractSlider* >& ref_sliders, bool ticks );
    void volumeChangeInternal(Volume& vol, QList< QAbstractSlider* >& ref_sliders );
    void updateInternal(Volume& vol, QList< QAbstractSlider* >& ref_sliders, bool muted);
#ifndef QT_NO_ACCESSIBILITY
    void updateAccesability();
#endif

    bool hasMuteButton() const				{ return (m_muteButton!=nullptr); }
    bool hasCaptureLED() const				{ return (m_captureButton!=nullptr); }

	QString calculatePlaybackIcon(MediaController::PlayState playState);
	QWidget *guiAddButtonSpacer();
	void guiAddCaptureButton(const QString &captureTooltipText);
	void guiAddMuteButton(const QString &muteTooltipText);
	void guiAddControlIcon(const QString &tooltipText);
	void guiAddControlLabel(Qt::Alignment alignment, const QString &channelName);
    QSize controlButtonSize();

    bool m_linked;

    QGridLayout *m_controlGrid;

	QLabel      *m_controlIcon;
	QLabel *m_controlLabel; // is either QLabel or VerticalText

	ToggleToolButton *m_muteButton;
	ToggleToolButton *m_captureButton;
	QToolButton *m_mediaPlayButton;
	QSize m_controlButtonSize;

    QMenu *m_moveMenu;

    QList<QAbstractSlider *> m_slidersPlayback;
    QList<QAbstractSlider *> m_slidersCapture;
    bool m_sliderInWork;
    int m_waitForSoundSetComplete;
    QList<int> volumeValues;
};

#endif
