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
#ifndef ViewSliders_h
#define ViewSliders_h

#include "viewbase.h"

class QBoxLayout;
class QGridLayout;
class QPushButton;
class KMessageWidget;

class Mixer;

#include "gui/viewbase.h"
#include "core/ControlManager.h"


class ViewSliders : public ViewBase
{
    Q_OBJECT

public:
    ViewSliders(QWidget *parent, const QString &id, Mixer *mixer, ViewBase::ViewFlags vflags, const QString &guiProfileId, KActionCollection *actColl);
    virtual ~ViewSliders();

    QWidget *add(const shared_ptr<MixDevice> md) override;
    void constructionFinished() override;
    void configurationUpdate() override;

public slots:
    void controlsChange(ControlManager::ChangeType changeType);

protected:
    void initLayout() override;
    Qt::Orientation orientationSetting() const override;

private:
    void refreshVolumeLevels() override;

    QGridLayout *m_layoutMDW;
    QBoxLayout *m_layoutSliders;
    QBoxLayout *m_layoutSwitches;
    QPushButton *m_configureViewButton;
    KMessageWidget *m_emptyStreamHint;
};

#endif
