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
#include <qlist.h>
#include <kcolordialog.h>

#include "colorwidget.h"
#include "kmixerwidget.h"

class Mixer;
class QTimer;


class ColorDialog : public ColorWidget {
   Q_OBJECT
  public:
   ColorDialog( QWidget * parent=0, const char * name=0, bool modal=FALSE, WFlags f=0 ) 
       : ColorWidget( parent, name, modal, f ) {
       connect( buttonApply, SIGNAL(clicked()), SLOT(apply()) );
   };

   virtual ~ColorDialog() {};

  protected slots:
   virtual void apply() { emit applied(); }
   virtual void accept() { ColorWidget::accept(); emit applied(); }
   virtual void reject() { ColorWidget::reject(); emit rejected(); }

  signals:
   void applied();
   void rejected();	
};


class KMixApplet : public KPanelApplet
{
   Q_OBJECT

  public:
   KMixApplet( const QString& configFile, Type t = Normal,
	       QWidget *parent = 0, const char *name = 0 );
   virtual ~KMixApplet();

   int widthForHeight(int height) const;
   int heightForWidth(int width) const; 
   
   void about();
   void help();
   void preferences();   

  protected slots:
   void triggerUpdateLayout();
   void updateLayoutNow(); 
   void selectMixer();
   void applyColors();
  
  protected:
   void resizeEvent( QResizeEvent * );
   void saveConfig();
    
  private:
   KMixerWidget *m_mixerWidget;
   QPushButton *m_errorLabel;
   QTimer *m_layoutTimer;
   int m_lockedLayout;
   ColorWidget *m_pref;
   bool insideOut; //  reverses direction of sliders and icon position
   void popupDirectionChange(Direction);

   static int s_instCount;
   static QList<Mixer> *s_mixers;
   static QTimer *s_timer;    

   KMixerWidget::Colors m_colors;
   bool m_customColors;
   
   int mixerNum;
   QString mixerName;
   Mixer *mixer;
};


#endif
