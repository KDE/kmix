//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef ViewDockAreaPopup_h
#define ViewDockAreaPopup_h

#include "viewbase.h"

#include <QIcon>
#include <QPushButton>

class QBoxLayout;
class QFrame;
class QGridLayout;
class QWidget;

class Mixer;
class MixDevice;
class KMixWindow;

class ViewDockAreaPopup : public ViewBase
{
    Q_OBJECT
public:
    ViewDockAreaPopup(QWidget* parent, QString id, ViewBase::ViewFlags vflags, QString guiProfileId, KMixWindow *dockW);
    virtual ~ViewDockAreaPopup();

    QWidget* add(shared_ptr<MixDevice> md) Q_DECL_OVERRIDE;
    void constructionFinished() Q_DECL_OVERRIDE;
    void refreshVolumeLevels() Q_DECL_OVERRIDE;
     void showContextMenu() Q_DECL_OVERRIDE;
     void configurationUpdate() Q_DECL_OVERRIDE;

protected:
    KMixWindow  *_kmixMainWindow;

    void wheelEvent ( QWheelEvent * e ) Q_DECL_OVERRIDE;
    void _setMixSet() Q_DECL_OVERRIDE;

private:
  QGridLayout* _layoutMDW;
    QPushButton* createRestoreVolumeButton ( int storageSlot );
    
    bool separatorBetweenMastersAndStreamsInserted;
    bool separatorBetweenMastersAndStreamsRequired;
    QFrame* seperatorBetweenMastersAndStreams;
    QBoxLayout* optionsLayout;
    QPushButton* configureViewButton;
    QPushButton *mainWindowButton;
    QPushButton *restoreVolumeButton1;
    QPushButton *restoreVolumeButton2;
    QPushButton *restoreVolumeButton3;
    QPushButton *restoreVolumeButton4;
    QIcon restoreVolumeIcon;

    static ProfControl* MatchAllForSoundMenu;
    static QString InternedString_Star;
    static QString InternedString_Subcontrols;

public slots:
       void controlsChange(int changeType);
       void configureView() Q_DECL_OVERRIDE;

private slots:
    void showPanelSlot();
	void resetRefs();
};

#endif

