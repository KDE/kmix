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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <klocale.h>
#include <kapp.h>
#include <kpopupmenu.h>

#include "kmixdockwidget.h"


KMixDockWidget::KMixDockWidget( QWidget *parent, const char *name )
   : KSystemTray( parent, name )
{
}

KMixDockWidget::~KMixDockWidget()
{
}

void KMixDockWidget::contextMenuAboutToShow( KPopupMenu* menu )
{
    for ( unsigned n=0; n<menu->count(); n++ )
    {
        if ( QString( menu->text( menu->idAt(n) ) )==i18n("Quit") )
            menu->removeItemAt( n );
    }

    menu->insertItem( i18n("Quit" ), kapp, SLOT(quit()) );
}

#include "kmixdockwidget.moc"
