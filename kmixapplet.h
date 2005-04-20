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

// Qt
#include <qlayout.h>
#include <qptrlist.h>
#include <qwidget.h>

// KDE
#include <kaboutdata.h>
#include <kdialogbase.h>
#include <kpanelapplet.h>

//KMix
#include "viewapplet.h"

class Mixer;
class ColorWidget;
class KMixApplet;


class AppletConfigDialog : public KDialogBase
{
  Q_OBJECT
  public:
   AppletConfigDialog( QWidget * parent=0, const char * name=0 );
   virtual ~AppletConfigDialog() {};

   void setActiveColors(const QColor& high, const QColor& low, const QColor& back);
   void activeColors(QColor& high, QColor& low, QColor& back) const;

   void setMutedColors(const QColor& high, const QColor& low, const QColor& back);
   void mutedColors(QColor& high, QColor& low, QColor& back) const;

   void setUseCustomColors(bool);
   bool useCustomColors() const;

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

   struct Colors {
       QColor high, low, back, mutedHigh, mutedLow, mutedBack;
   };

   void about();
   void help();
   void preferences();   
   void reportBug();
   void paletteChange ( const QPalette & oldPalette );

   QSize sizeHint() const;
   QSizePolicy sizePolicy() const;
   int widthForHeight(int) const;
   int heightForWidth(int) const;

protected slots:
   void selectMixer();
   void applyPreferences();
   void preferencesDone();
   void updateGeometrySlot();
  
protected:
   void resizeEvent( QResizeEvent * );
   void saveConfig();
   void saveConfig( KConfig *config, const QString &grp );
   void loadConfig();
   void loadConfig( KConfig *config, const QString &grp );

private:
   void positionChange(Position);
   void setColors();
   void setColors( const Colors &color );

   ViewApplet *m_mixerWidget;
   QPushButton *m_errorLabel;
   AppletConfigDialog *m_pref;

   static int             s_instCount;
   Mixer                  *_mixer;

   KMixApplet::Colors _colors;
   bool               _customColors;

   QLayout* _layout;

   QString _mixerId;
   QString _mixerName;

   KAboutData m_aboutData;
};


#endif
