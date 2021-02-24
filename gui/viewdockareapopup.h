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

#include "core/ControlManager.h"

class QBoxLayout;
class QFrame;
class QGridLayout;
class QWidget;

class Mixer;
class MixDevice;
class KMixWindow;
class KXmlGuiWindow;


class ViewDockAreaPopup : public ViewBase
{
    Q_OBJECT

public:
    ViewDockAreaPopup(QWidget* parent, const QString &id, ViewBase::ViewFlags vflags, const QString &guiProfileId, KXmlGuiWindow *dockW);
    virtual ~ViewDockAreaPopup();

    QWidget* add(shared_ptr<MixDevice> md) override;
    void constructionFinished() override;
    void refreshVolumeLevels() override;
     void showContextMenu() override;
     void configurationUpdate() override;

protected:
    void initLayout() override;
    Qt::Orientation orientationSetting() const override;

    void keyPressEvent(QKeyEvent *ev) override;

private:
    KXmlGuiWindow *_kmixMainWindow;
    QGridLayout *_layoutMDW;

    QPushButton* createRestoreVolumeButton (int storageSlot);
    void resetRefs();
    
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

public slots:
    void controlsChange(ControlManager::ChangeType changeType);
    void configureView() override;
    void showPanelSlot();
};

#endif

