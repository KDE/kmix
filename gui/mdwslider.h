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

#include "volumeslider.h"
#include <QCheckBox>
#include <QList>
#include <QWidget>
#include <qlist.h>
#include <qpixmap.h>

class QBoxLayout;
class QGridLayout;
class QToolButton;
class QLabel;
class QMenu;

#include <kiconloader.h>

class MixDevice;
class VerticalText;
class ViewBase;

#include "gui/mixdevicewidget.h"
#include "core/volume.h"


class MDWSlider : public MixDeviceWidget
{
    Q_OBJECT

public:
    MDWSlider( shared_ptr<MixDevice> md,
	       bool includePlayback, bool includeCapture,
	       bool includeMixerName, bool small, Qt::Orientation,
	       QWidget* parent, ViewBase* view, ProfControl *pctl);
    virtual ~MDWSlider();

    enum LabelType { LT_ALL, LT_FIRST_CAPTURE, LT_NONE };
    void addActionToPopup( QAction *action );
    void createActions();
    void createShortcutActions();
    
    // GUI
    bool isStereoLinked() const Q_DECL_OVERRIDE { return m_linked; }
    void setStereoLinked( bool value ) Q_DECL_OVERRIDE;
    void setLabeled( bool value ) Q_DECL_OVERRIDE;
    void setTicks( bool ticks ) Q_DECL_OVERRIDE;
    void setIcons( bool value ) Q_DECL_OVERRIDE;

    QToolButton* addMediaButton(QString iconName, QLayout* layout, QWidget *parent);
    void updateMediaButton();
    void setColors( QColor high, QColor low, QColor back ) Q_DECL_OVERRIDE;
    void setMutedColors( QColor high, QColor low, QColor back ) Q_DECL_OVERRIDE;
    
    bool eventFilter(QObject *obj, QEvent *ev) Q_DECL_OVERRIDE;

    QString iconName();
    // Layout
    QSizePolicy sizePolicy() const;
	QSize sizeHint() const Q_DECL_OVERRIDE;
	int labelExtentHint() const;
	void setLabelExtent(int extent);
	bool hasMuteButton() const;
	bool hasCaptureLED() const;

	static VolumeSliderExtraData DummVolumeSliderExtraData;
	static bool debugMe;
    
    
public slots:
    void toggleRecsrc();
    void toggleMuted();
    void toggleStereoLinked();

    void setDisabled( bool value ) Q_DECL_OVERRIDE;
    void update() Q_DECL_OVERRIDE;
    void showMoveMenu();
    void showContextMenu( const QPoint &pos = QCursor::pos() ) Q_DECL_OVERRIDE;
    void increaseOrDecreaseVolume(bool arg1, Volume::VolumeTypeFlag volumeType);
    VolumeSliderExtraData& extraData(QAbstractSlider *slider);
    void addMediaControls(QBoxLayout* arg1);

private slots:
    void setRecsrc(bool value);
    void setMuted(bool value);
    void volumeChange( int );
    void sliderPressed();
    void sliderReleased();

    void increaseVolume();
    void decreaseVolume();

    void moveStreamAutomatic();
    void moveStream( QString destId );

    void mediaPlay(bool);
    void mediaNext(bool);
    void mediaPrev(bool);

private:
    void setIcon(const QString &filename, QWidget *label);
//    QPixmap loadIcon( QString filename, KIconLoader::Group group );
    void createWidgets( bool showMuteLED, bool showCaptureLED, bool includeMixer );
    void addSliders( QBoxLayout *volLayout, char type, Volume& vol,
                     QList<QAbstractSlider *>& ref_sliders, QString tooltipText );
    //void addDefaultLabel(QBoxLayout *layout, Qt::Orientation orientation);

    // Methods that are called two times from a wrapper. Once for playabck, once for capture
    void setStereoLinkedInternal( QList< QAbstractSlider* >& ref_sliders, bool showSubcontrolLabels);
    void setTicksInternal( QList< QAbstractSlider* >& ref_sliders, bool ticks );
    void volumeChangeInternal(Volume& vol, QList< QAbstractSlider* >& ref_sliders );
    void updateInternal(Volume& vol, QList< QAbstractSlider* >& ref_sliders, bool muted);
#ifndef QT_NO_ACCESSIBILITY
    void updateAccesability();
#endif

	QString calculatePlaybackIcon(MediaController::PlayState playState);
	QWidget *guiAddButtonSpacer();
	void guiAddCaptureButton(const QString &captureTooltipText);
	void guiAddMuteButton(const QString &muteTooltipText);
	void guiAddControlIcon(const QString &tooltipText);
	void guiAddControlLabel(Qt::Alignment alignment, const QString &channelName);
	void addGlobalShortcut(QAction* action, const QString& label, bool dynamicControl);
    QSize controlButtonSize();

    bool m_linked;

    QGridLayout *m_controlGrid;

	QLabel      *m_controlIcon;
	QLabel *m_controlLabel; // is either QLabel or VerticalText

	QToolButton* m_muteButton;
	QCheckBox* m_captureButton;
	QToolButton *m_mediaPlayButton;
	QSize m_controlButtonSize;

    KActionCollection*   _mdwMoveActions;
    QMenu *m_moveMenu;

    QList<QAbstractSlider *> m_slidersPlayback;
    QList<QAbstractSlider *> m_slidersCapture;
    bool m_sliderInWork;
    int m_waitForSoundSetComplete;
    QList<int> volumeValues;
};

#endif
