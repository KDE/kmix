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

class QBoxLayout;
#include <QFrame>
#include <QHash>
class QLabel;
#include <QPushButton>
class QWidget;

class Mixer;
#include "viewbase.h"

class ViewSliders : public ViewBase
{
    Q_OBJECT
public:
    ViewSliders(QWidget* parent, QString id, Mixer* mixer, ViewBase::ViewFlags vflags, QString guiProfileId, KActionCollection *actColl);
    virtual ~ViewSliders();

    virtual QWidget* add(shared_ptr<MixDevice>);
    virtual void constructionFinished();
    virtual void configurationUpdate();

public slots:
    void controlsChange(int changeType);

protected:
    virtual void _setMixSet();

private:
    virtual void refreshVolumeLevels();

    QBoxLayout* _layoutMDW;
    QLayout* _layoutSliders;
	QBoxLayout* _layoutEnum;
    QHash<QString,QFrame*> _separators;
    QPushButton* _configureViewButton;
    QLabel* emptyStreamHint;
};

#endif

