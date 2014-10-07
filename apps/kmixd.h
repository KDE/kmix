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

  private:
   bool m_multiDriverMode;

   QString m_hwInfoString;
   QString m_defaultCardOnStart;
   unsigned int m_configVersion;
   QList<QString> m_backendFilter;

  private slots:
   void delayedInitialization();
   void saveConfig();
   void plugged( const char* driverName, const QString& udi, QString& dev);
   void unplugged( const QString& udi);
};

#endif // KMIXD_H
