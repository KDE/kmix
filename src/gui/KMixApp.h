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

#include <kuniqueapplication.h>

const QString KMIX_DBUS_SERVICE = "org.kde.kmixd";
const QString KMIX_DBUS_PATH = "/KMixD";

class OrgKdeKMixKMixDInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef ::OrgKdeKMixKMixDInterface KMixD;
        }
    }
}

class KMixWindow;
class KMixOSD;
class KStatusNotifierItem;

class KMixApp : public KUniqueApplication
{
Q_OBJECT
 public:
    KMixApp();
    ~KMixApp();
    int newInstance();
    static org::kde::KMix::KMixD *daemon();
 private:
    static org::kde::KMix::KMixD *s_daemon;
    KMixWindow *m_kmix;
    KStatusNotifierItem *m_icon;
    KMixOSD *m_osd;
};

#endif
