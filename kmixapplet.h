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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KMIXAPPLET_H
#define KMIXAPPLET_H

#include <qwidget.h>
#include <kaction.h>
#include <kdialogbase.h>
#include <kpanelapplet.h>
#include <qptrlist.h>
#include <kcolordialog.h>

#include "kmixerwidget.h"

class Mixer;
class QTimer;
class ColorWidget;


class AppletConfigDialog : public KDialogBase
{
  Q_OBJECT
  public:
   AppletConfigDialog( QWidget * parent=0, const char * name=0 );
   virtual ~AppletConfigDialog() {};

   void setActiveColors(const QColor& high, const QColor& low, const QColor& back);
   void activeColors(QColor& high, QColor& low, QColor& back);

   void setMutedColors(const QColor& high, const QColor& low, const QColor& back);
   void mutedColors(QColor& high, QColor& low, QColor& back);

   void setUseCustomColors(bool);
   bool useCustomColors();

   void setReverseDirection(bool);
   bool reverseDirection();

  protected slots:
   virtual void slotOk();
   virtual void slotApply();

  signals:
   void applied();
  private:
   ColorWidget* colorWidget;
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
   void applyPreferences();
   void preferencesDone();
  
  protected:
   void resizeEvent( QResizeEvent * );
   void saveConfig();
    
  private:
   KMixerWidget *m_mixerWidget;
   QPushButton *m_errorLabel;
   QTimer *m_layoutTimer;
   int m_lockedLayout;
   AppletConfigDialog *m_pref;
   bool reversedDir; //  reverses direction of sliders and icon position
   void popupDirectionChange(Direction);
   Direction checkReverse(Direction);
   void setColors();

   static int s_instCount;
   static QPtrList<Mixer> *s_mixers;
   static QTimer *s_timer;    

   KMixerWidget::Colors m_colors;
   bool m_customColors;
   
   int mixerNum;
   QString mixerName;
   Mixer *mixer;
};


#endif
