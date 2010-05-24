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


#include "mixer.h"
#include "mixdevicewidget.h"

// QT
class QSlider;


// KDE
class KActionCollection;
class KConfig;
//class KTabWidget;

// KMix
class GUIProfile;
class Mixer;
#include "viewbase.h"
// KMix experimental


class KMixerWidget : public QWidget
{
   Q_OBJECT

  public:
   explicit KMixerWidget( Mixer *mixer,
                          QWidget *parent=0, const char *name=0, ViewBase::ViewFlags vflags=0, GUIProfile* guiprof=0, ProfTab* tab = 0, KActionCollection* coll = 0 );
   ~KMixerWidget();

   Mixer *mixer() { return _mixer; }
   ViewBase* currentView();
   bool generationIsOutdated();
   static void increaseGeneration();
   QString id() { return m_id; }

   
  signals:
   void toggleMenuBar();
   void rebuildGUI();
   void redrawMixer( const QString& mixer_ID );
    
  public slots:
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void toggleMenuBarSlot();

   void saveConfig( KConfig *config );
   void loadConfig( KConfig *config );

  private slots:
   void balanceChanged(int balance);

  private:
   Mixer *_mixer;
   QString m_id;
   QSlider *m_balanceSlider;
   QVBoxLayout *m_topLayout; // contains TabWidget and balance slider
   GUIProfile* _guiprof;
   ProfTab* _tab;
   std::vector<ViewBase*> _views;
   KActionCollection* _actionCollection;  // -<- applciations wide action collection

   int _generation;
   static int _currentGeneration;
   
   void createLayout(ViewBase::ViewFlags vflags);
   bool possiblyAddView(ViewBase* vbase);
   void createViewsByProfile(Mixer* mixer, GUIProfile *guiprof, QString tabId, ViewBase::ViewFlags vflags);
};

#endif
