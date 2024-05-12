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
#ifndef KMixApp_h
#define KMixApp_h

#include <QMutex>
#include <QObject>
#include <QPointer>

class QCommandLineParser;
class KMixWindow;


class KMixApp : public QObject
{
    Q_OBJECT

 public:
    KMixApp();
    virtual ~KMixApp();

    void parseOptions(const QCommandLineParser &parser);

   // Options parsed from the command line and passed down to
   // the KMixWindow via this KMixApp.
   enum StartupOption
   {
       KeepVisibility = 0x01,
       FailsafeReset = 0x02,
       NoSystemTray = 0x04,
   };
   Q_DECLARE_FLAGS(StartupOptions, StartupOption)

public Q_SLOTS:
    void newInstance(const QStringList &arguments = QStringList(), const QString &workingDirectory = QString());

private:
    bool restoreSessionIfApplicable();
    void createWindowOnce();

    QPointer<KMixWindow> m_kmix;
    QRecursiveMutex creationLock;
    KMixApp::StartupOptions m_startupOptions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KMixApp::StartupOptions)

#endif
