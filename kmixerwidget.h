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
class KActionMenu;
class KConfig;
class KTabWidget;

// KMix
class GUIProfile;
class Mixer;
#include "viewbase.h"
// KMix experimental


class KMixerWidget : public QWidget  
{
   Q_OBJECT

  public:
   KMixerWidget( Mixer *mixer,
                 QWidget *parent=0, const char *name=0, ViewBase::ViewFlags vflags=0 );
   ~KMixerWidget();
	
   Mixer *mixer() const { return _mixer; }

/* !!! This id() is rubbish. It is not guaranteed, that it is the same on every KMix start.
   !!! Still it is used for constructing config file group names :-(((
*/
  const QString& id() const;
   KActionCollection* getActionCollection() const { return 0; /* m_actions; */ }
	
  signals:
   void masterMuted( bool );
   void newMasterVolume(Volume vol);
   void toggleMenuBar();
   void activateNextlayout();

  public slots:
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void toggleMenuBarSlot();
   void nextLayout();

   void saveConfig( KConfig *config, const QString &grp );
   void loadConfig( KConfig *config, const QString &grp );

  private slots:
      //void updateBalance();
      void balanceChanged(int balance);

  private:
   Mixer *_mixer;
   QString m_id;
   QSlider *m_balanceSlider;
   QVBoxLayout *m_topLayout; // contains the Card selector, TabWidget and balance slider

   KTabWidget* m_ioTab;

	std::vector<ViewBase*> _views;

   KActionMenu *m_toggleMixerChannels;

   bool _iconsEnabled;
   bool _labelsEnabled;
   bool _ticksEnabled;

   void createLayout(ViewBase::ViewFlags vflags);
   void possiblyAddView(ViewBase* vbase);
   void createViewsByProfile(Mixer* mixer, GUIProfile* guiprof, ViewBase::ViewFlags vflags);
};

#endif
