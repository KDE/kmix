/*
 * KMix -- KDE's full featured mini mixer
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

#ifndef KMIXD_H
#define KMIXD_H


#include <config.h>

// Qt
#include <QString>
#include <QtDBus/QtDBus> 

#include <qlist.h>
#include <QTimer>

// KDE
//class KAccel;
class KAction;
//#include <kxmlguiwindow.h>
#include <kdedmodule.h> 
	
// KMix
//class KMixPrefDlg;
//class KMixDockWidget;
//class KMixWindow;
//class ViewDockAreaPopup;
#include "core/mixer.h"

//class OSDWidget;

class
KMixD : public KDEDModule, protected QDBusContext
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.KMixD")

  public:
   KMixD(QObject* parent, const QList<QVariant>&);
   ~KMixD();

  private:
   void saveBaseConfig();
   void loadConfig();
   void loadBaseConfig();

   void initActions();
   void initActionsLate();

   void fixConfigAfterRead();

  public slots:
   //void loadVolumes();
   void saveVolumes();
   //virtual void applyPrefs( KMixPrefDlg *prefDlg );

  private:
   //KAccel *m_keyAccel;
   //KAction* _actionShowMenubar;

   bool m_multiDriverMode;         // Not officially supported.
   bool m_autouseMultimediaKeys;   // Due to message freeze, not in config dialog in KDE4.4

   QString m_hwInfoString;
   QString m_defaultCardOnStart;
   bool m_dontSetDefaultCardOnStart;
   unsigned int m_configVersion;
   //void increaseOrDecreaseVolume(bool increase);
   QList<QString> m_backendFilter;

  private slots:
   void saveConfig();
   //void slotHWInfo();
   void plugged( const char* driverName, const QString& udi, QString& dev);
   void unplugged( const QString& udi);
   //void slotIncreaseVolume();
   //void slotDecreaseVolume();
   //void slotMute();
};

#endif // KMIXD_H
