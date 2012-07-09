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

#ifndef KMIX_H
#define KMIX_H


#include <config.h>

// Qt

// KDE
#include <kxmlguiwindow.h>

// KMix
class OrgKdeKMixMixSetInterface;
namespace org {
    namespace kde {
        namespace KMix {
            typedef ::OrgKdeKMixMixSetInterface MixSet;
        }
    }
}

class KMixDockWidget;

class
KMixWindow : public KXmlGuiWindow
{
   Q_OBJECT

  public:
   KMixWindow(bool invisible);
   ~KMixWindow();

    private:
    org::kde::KMix::MixSet *m_mixers;
    KMixDockWidget *m_dockWidget;
};

#endif // KMIX_H
