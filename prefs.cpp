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
  addTab( page1,i18n("General") );
  addTab( page2,i18n("Channels") );

  // Define page 1
  QButtonGroup *grpbox1a = new QButtonGroup(i18n("Startup settings"), page1 );
  grpbox1a->setGeometry( 10, 10, page1->width()-20, page1->height()-20 );

  int x=10, y=20;
  menubarChk = new QCheckBox(grpbox1a);
  menubarChk->setText(i18n("Menubar"));
  menubarChk->setGeometry(x,y, grpbox1a->width()-20, menubarChk->height() );

  y += (menubarChk->height() );
  tickmarksChk = new QCheckBox(grpbox1a);
  tickmarksChk->setText(i18n("Tickmarks"));
  tickmarksChk->setGeometry(x,y, grpbox1a->width()-20, tickmarksChk->height() );

  y += tickmarksChk->height();
  dockingChk = new QCheckBox(grpbox1a);
  dockingChk->setText(i18n("Allow docking"));
  dockingChk->setGeometry(x,y, grpbox1a->width()-20, dockingChk->height() );

  y += dockingChk->height();
  grpbox1a->setGeometry( 10, 10, page1->width()-20, y+10);
  // Define page 2
  createChannelConfWindow();

  setCancelButton();
  setApplyButton();
  setOkButton();


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));

  int maxheight = grpbox1a->height();
  if ( maxheight < grpbox2a->height() )
    maxheight = grpbox2a->height();

  page1->setFixedSize(page1->width(),maxheight+10);
  page2->setFixedSize(page1->width(),maxheight+10);
  grpbox1a->resize(grpbox1a->width(),maxheight);
  grpbox2a->resize(grpbox2a->width(),maxheight);

  setCaption(i18n("KMix Preferences") );
}




void Preferences::createChannelConfWindow()
{
  bool created = false;

  grpbox2a = new QGroupBox (i18n("Mixer channel setup (not saved yet)"),page2);
  QLabel *qlb;

  int ypos=20;
  int x1=10,x2=120, x3=160;
  qlb = new QLabel(grpbox2a);
  qlb->setText(i18n("Device"));
  qlb->move(x1,ypos);
  qlb = new QLabel(grpbox2a);
  qlb->setText(i18n("Show"));
  qlb->move(x2,ypos);
  qlb = new QLabel(grpbox2a);
  qlb->setText(i18n("Split"));
  qlb->move(x3,ypos);
  ypos += qlb->height();

  for  (MixDevice *mdev = mix->First ; mdev ;  mdev = mdev->Next  ) {
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

    QCheckBox *qcbSplit;
    if (mdev->is_stereo) {
      qcbSplit = new QCheckBox(grpbox2a);
      qcbSplit->move(x3,ypos);
      if (mdev->StereoLink)
	qcbSplit->setChecked(false);
      else
	qcbSplit->setChecked(true);
    }
    else
      qcbSplit = NULL;

    cSetup.append(new ChannelSetup(mdev->device_num,qle,qcb,qcbSplit));
    ypos += qle->height();
  }

  grpbox2a->setGeometry( 10, 10, page2->width()-20, ypos+10);

  created = true;
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

