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
class Mixer;
class QPushButton;
class QTimer;

class KMixApplet : public KPanelApplet
{
   Q_OBJECT

  public:
   KMixApplet( Mixer *mixer, QWidget *parent = 0, const char* name = 0 );

   KMixerWidget *mixerWidget() { return m_mixerWidget; };
   void setMixerWidget( KMixerWidget *mw );

   int widthForHeight(int height);
   int heightForWidth(int width); 
   void removedFromPanel();

  signals:
   void closeApplet( KMixApplet *applet );
   void clickedButton();

  protected slots:
   void updateSize() { updateLayout(); }
   void showButton() { emit clickedButton(); };
   void updateLayout();

  protected:
   void resizeEvent( QResizeEvent * );
    
  private:
   KMixerWidget *m_mixerWidget;
   QButton *m_button;
   QTimer *m_layoutTimer;
   int m_lockedLayout;
};


#endif
