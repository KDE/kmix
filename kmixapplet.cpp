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

#include <stdlib.h>

#include <qlayout.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kaction.h>
#include <qpushbutton.h>

#include "kmixerwidget.h"
#include "mixer.h"
#include "mixdevicewidget.h"
#include "kmixapplet.h"


KMixApplet::KMixApplet( KMixerWidget *mixerWidget, QWidget *parent, const char* name )
   : KPanelApplet( parent, name )
{
   // init mixer widget
   m_mixerWidget = mixerWidget;
   m_mixerWidget->reparent( this, QPoint(0, 0), TRUE );

   connect( m_mixerWidget, SIGNAL(updateLayout()), this, SLOT(updateSize()));
}

int KMixApplet::widthForHeight(int )
{
  return m_mixerWidget->minimumSize().width();
}
 
int KMixApplet::heightForWidth(int )
{
  return m_mixerWidget->minimumSize().height();
}
 
void KMixApplet::resizeEvent( QResizeEvent *e )
{
   KPanelApplet::resizeEvent(e);
   m_mixerWidget->resize( width(), height() );
}           

void KMixApplet::removedFromPanel()
{
   emit closeApplet( this );
}
