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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KMIXERWIDGET_H
#define KMIXERWIDGET_H

#include <vector>

#include <qwidget.h>
#include <qptrlist.h>
class QString;
class QGridLayout;

#include <kpanelapplet.h>
class KPopupMenu;

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
class Mixer;
#include "viewbase.h"
class ViewInput;
class ViewOutput;
class ViewSwitches;
// KMix experimental
class ViewGrid;
class ViewSurround;


class KMixerWidget : public QWidget  
{
   Q_OBJECT

  public:
   KMixerWidget( int _id, Mixer *mixer, const QString &mixerName,
                 MixDevice::DeviceCategory categoryMask = MixDevice::ALL ,
                 QWidget *parent=0, const char *name=0, ViewBase::ViewFlags vflags=0 );
   ~KMixerWidget();
	
   enum KMixerWidgetIO { OUTPUT=0, INPUT };

   const Mixer *mixer() const { return _mixer; };

  int id() const { return m_id; };

   KActionCollection* getActionCollection() const { return 0; /* m_actions; */ }
	
  signals:
   void masterMuted( bool );
   void newMasterVolume(Volume vol);
   void toggleMenuBar();

  public slots:
   void setTicks( bool on );
   void setLabels( bool on );
   void setIcons( bool on );
   void setValueStyle( int vs );
   void toggleMenuBarSlot();

   void saveConfig( KConfig *config, const QString &grp );
   void loadConfig( KConfig *config, const QString &grp );

  private slots:
      //void updateBalance();
      void balanceChanged(int balance);

  private:
   Mixer *_mixer;
   QSlider *m_balanceSlider;
   QVBoxLayout *m_topLayout; // contains the Card selector, TabWidget and balance slider

   KTabWidget* m_ioTab;

	std::vector<ViewBase*> _views;
   int m_id;

   KActionMenu *m_toggleMixerChannels;

   bool _iconsEnabled;
   bool _labelsEnabled;
   bool _ticksEnabled;
   int _valueStyle;
   MixDevice::DeviceCategory m_categoryMask;

   void createLayout(ViewBase::ViewFlags vflags);
   void possiblyAddView(ViewBase* vbase);
};

#endif
