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

#ifndef KMIXAPPLET_H
#define KMIXAPPLET_H

#include <qwidget.h>
#include <kaction.h>
#include <kpanelapplet.h>

class KMixerWidget;

class KMixApplet : public KPanelApplet
{
   Q_OBJECT

  public:
   KMixApplet( KMixerWidget *mixerWidget, QWidget *parent = 0, const char* name = 0 );

   KMixerWidget *mixerWidget() { return m_mixerWidget; };

   int widthForHeight(int height);
   int heightForWidth(int width); 
   void removedFromPanel();

  signals:
   void closeApplet( KMixApplet *applet );

  protected slots:
   void updateSize() { updateLayout(); }

  protected:
   void resizeEvent( QResizeEvent* );

  private:
   KMixerWidget *m_mixerWidget;
};


#endif
