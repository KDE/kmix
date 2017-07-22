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
class QToolButton;
class QLabel;

class KAction;
class QMenu;
#include <kshortcut.h>
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
    void addActionToPopup( KAction *action );
    void createActions();
    void createShortcutActions();
    
    // GUI
    bool isStereoLinked() const Q_DECL_OVERRIDE { return m_linked; }
    void setStereoLinked( bool value ) Q_DECL_OVERRIDE;
    void setLabeled( bool value ) Q_DECL_OVERRIDE;
    void setTicks( bool ticks ) Q_DECL_OVERRIDE;
    void setIcons( bool value ) Q_DECL_OVERRIDE;
//    void setIcon( QString filename, QLabel** label );
    void setIcon( QString filename, QWidget* label );
    QToolButton* addMediaButton(QString iconName, QLayout* layout, QWidget *parent);
    void updateMediaButton();
    void setColors( QColor high, QColor low, QColor back ) Q_DECL_OVERRIDE;
    void setMutedColors( QColor high, QColor low, QColor back ) Q_DECL_OVERRIDE;
    
    bool eventFilter( QObject* obj, QEvent* e ) Q_DECL_OVERRIDE;
    QString iconName();
    // Layout
    QSizePolicy sizePolicy() const;
	QSize sizeHint() const Q_DECL_OVERRIDE;
	int labelExtentHint() const;
	void setLabelExtent(int extent);
	bool hasMuteButton() const;
	void setMuteButtonSpace(bool);
	void setCaptureLEDSpace(bool);
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
    void setRecsrc( bool value );
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
    QPixmap loadIcon( QString filename, KIconLoader::Group group );
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

    QWidget* createLabel(QWidget* parent, QString& label, QBoxLayout *layout, bool);
	QString calculatePlaybackIcon(MediaController::PlayState playState);
	void guiAddSlidersAndMediacontrols(bool playSliders, bool capSliders, bool mediaControls, QBoxLayout* layout, const QString& tooltipText, const QString& captureTooltipText);
	void guiAddCaptureCheckbox(bool wantsCaptureLED, const Qt::Alignment& alignmentForCapture,
		QBoxLayout* layoutForCapture, const QString& captureTooltipText);
	void guiAddMuteButton(bool wantsMuteButton, Qt::Alignment alignment, QBoxLayout* layoutForMuteButton, const QString& muteTooltipText);
	void guiAddControlIcon(Qt::Alignment alignment, QBoxLayout* layout, const QString& tooltipText);
	void addGlobalShortcut(QAction* action, const QString& label, bool dynamicControl);

    bool m_linked;

	QWidget *muteButtonSpacer;
	QWidget *captureSpacer;
	QWidget *labelSpacer;

    // GUI: Top portion ( Icon + Mute)
	QLabel      *m_iconLabelSimple;
	QToolButton* m_qcb;
	QLabel* m_muteText;
        
	QLabel *m_label; // is either QLabel or VerticalText
	QToolButton *mediaButton;

	QCheckBox* m_captureCheckbox;
    QLabel* m_captureText;

	int labelSpacing;
	bool muteButtonSpacing;
	bool captureLEDSpacing;

    KActionCollection*   _mdwMoveActions;
    QMenu *m_moveMenu;

    QList<QAbstractSlider *> m_slidersPlayback;
    QList<QAbstractSlider *> m_slidersCapture;
    bool m_sliderInWork;
    int m_waitForSoundSetComplete;
    QList<int> volumeValues;
};

#endif
