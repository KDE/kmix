/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
 *                        esken@kde.org
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

#include "sets.h"
#include "prefs.h"
#include "prefs.moc"
#include "kmix.h"
#include <klocale.h>
#include <qchkbox.h> 
#include <qlabel.h>

Preferences::Preferences( QWidget *parent, Mixer *mix ) :
   QTabDialog( parent )
{
  resize(300,400);
  this->mix = mix;
  grpbox2a  = NULL;

  page1 = new QWidget( this );
  page1->setGeometry(10,10,width()-20,height()-20);
  page2 = new QWidget( this );
  page2->setGeometry(10,10,width()-20,height()-20);
  addTab( page1, klocale->translate("General") );
  addTab( page2, klocale->translate("Channels") );

  // Define page 1
  QButtonGroup *grpbox1a = new QButtonGroup( klocale->translate("Startup settings"), page1 );
  grpbox1a->setGeometry( 10, 10, page1->width()-20, page1->height()-20 );

  int x=10, y=20;
  menubarChk = new QCheckBox(grpbox1a);
  menubarChk->setText(klocale->translate("Menubar"));
  menubarChk->setGeometry(x,y, grpbox1a->width()-20, menubarChk->height() );

  y += (menubarChk->height() );
  tickmarksChk = new QCheckBox(grpbox1a);
  tickmarksChk->setText(klocale->translate("Tickmarks"));
  tickmarksChk->setGeometry(x,y, grpbox1a->width()-20, tickmarksChk->height() );

  y += tickmarksChk->height();
  grpbox1a->setGeometry( 10, 10, page1->width()-20, y+10);
  // Define page 2
  updateChannelConfWindow();

  setCancelButton();
  setApplyButton();
  setOkButton();


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));

  int maxheight = grpbox1a->height();
  if ( maxheight < grpbox2a->height() )
    maxheight = grpbox2a->height();

  page1->setFixedSize(page1->width(),maxheight+20);
  page2->setFixedSize(page1->width(),maxheight+20);
  grpbox1a->resize(grpbox1a->width(),maxheight);
  grpbox2a->resize(grpbox2a->width(),maxheight);

  setCaption( klocale->translate("KMix Preferences") );
}


void Preferences::updateChannelConfWindow()
{
  grpbox2a = new QGroupBox (klocale->translate("Mixer channel setup (not saved yet)"),page2);
  MixDevice *mdev = mix->First;
  QLabel *qlb;

  int ypos=20;
  int x1=10,x2=120;
  qlb = new QLabel(grpbox2a);
  qlb->setText(klocale->translate("Device"));
  qlb->move(x1,ypos);
  qlb = new QLabel(grpbox2a);
  qlb->setText(klocale->translate("Show"));
  qlb->move(x2,ypos);
  ypos += qlb->height();

  while (mdev) {

    /// TODO: Create an array, where qle's are inserted. Dann bei "apply"
    /// das qle Array durchgehen und neue Namen setzen.

    QLineEdit *qle;
    qle = new  QLineEdit(grpbox2a,mdev->devname);
    qle->setText(mdev->devname);
    qle->move(x1,ypos);
    QCheckBox *qcb = new QCheckBox(grpbox2a);
    qcb->move(x2,ypos);
    if (mdev->is_disabled)
      qcb->setChecked(false);
    else
      qcb->setChecked(true);

    ypos += qle->height();
    mdev = mdev->Next;
  }

  grpbox2a->setGeometry( 10, 10, page2->width()-20, ypos+10);
}      


void Preferences::slotShow()
{
  show();
}

void Preferences::slotOk()
{
  slotApply();
  hide();
}

void Preferences::slotApply()
{
  emit optionsApply();
}

void Preferences::slotCancel()
{
  hide();
}


