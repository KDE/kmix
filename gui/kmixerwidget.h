/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
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

#ifndef KMIXERWIDGET_H
#define KMIXERWIDGET_H

#include <vector>

#include <QWidget>
class QString;


#include "core/ControlManager.h"
#include "core/mixer.h"
#include "gui/mixdevicewidget.h"

// Qt
#include <QVBoxLayout>

// KDE
class KActionCollection;
class KConfig;

// KMix
class GUIProfile;
class ProfTab;
class Mixer;
#include "viewbase.h"
// KMix experimental


class KMixerWidget : public QWidget
{
   Q_OBJECT

  public:
   explicit KMixerWidget( Mixer *mixer,
                          QWidget *parent, ViewBase::ViewFlags vflags, const QString  &guiprofId, KActionCollection* coll = nullptr );
   virtual ~KMixerWidget();

    Mixer *mixer() const			{ return (_mixer); }
    GUIProfile *getGuiprof() const		{ return (GUIProfile::find(_guiprofId)); }
    ViewBase *currentView() const;
   
  public Q_SLOTS:
   void setIcons( bool on );

   void saveConfig( KConfig *config );
   void loadConfig( KConfig *config );

  private:
   Mixer *_mixer;
   QVBoxLayout *m_topLayout; // contains TabWidget
   QString _guiprofId;
   ProfTab* _tab;
   std::vector<ViewBase*> _views;
   KActionCollection* _actionCollection;  // -<- applications wide action collection
   
   void createLayout(ViewBase::ViewFlags vflags);
   bool possiblyAddView(ViewBase* vbase);
};

#endif
