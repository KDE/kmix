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

//static char rcsid[]="$Id$";

Preferences::Preferences( QWidget *parent, Mixer *mix ) :
   QTabDialog( parent )
{
  //resize(300,400);
  this->mix = mix;
  grpbox2a  = NULL;

  page1 = new QWidget( this );
  //page1->setGeometry(10,10,width()-20,height()-20);
  page2 = new QWidget( this );
  //page2->setGeometry(10,10,width()-20,height()-20);

  // Define page 1
  QButtonGroup *grpbox1a = new QButtonGroup(i18n("Startup settings"), page1 );
  grpbox1a->move( 10, 10 ); //, page1->width()-20, page1->height()-20 );

  int x=10, y=20, maxwidth=0;
  menubarChk = new QCheckBox(grpbox1a);
  menubarChk->setText(i18n("Menubar"));
  menubarChk->move(x,y); // , grpbox1a->width()-20, menubarChk->height() );
  if (menubarChk->width() > maxwidth ) maxwidth = menubarChk->width();

  y += (menubarChk->height() );
  tickmarksChk = new QCheckBox(grpbox1a);
  tickmarksChk->setText(i18n("Tickmarks"));
  tickmarksChk->move(x,y);
  if (tickmarksChk->width() > maxwidth ) maxwidth = tickmarksChk->width();

  y += tickmarksChk->height();
  dockingChk = new QCheckBox(grpbox1a);
  dockingChk->setText(i18n("Allow docking"));
  dockingChk->move(x,y);
  if (dockingChk->width() > maxwidth ) maxwidth = dockingChk->width();

  y += dockingChk->height();
  grpbox1a->setGeometry( 10, 10, maxwidth+20, y+10);
  // Define page 2
  createChannelConfWindow();

  setCancelButton(i18n("Cancel"));
  setApplyButton(i18n("Apply"));
  setOkButton(i18n("OK"));


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));

  int maxheight = grpbox1a->height();
  if ( maxheight < grpbox2a->height() )
    maxheight = grpbox2a->height();
  maxwidth =grpbox1a->width();
  if ( maxwidth < grpbox2a->width() )
    maxwidth = grpbox2a->width();

  grpbox1a->setFixedSize(maxwidth,maxheight);
  grpbox2a->setFixedSize(maxwidth,maxheight);
  page1->setFixedSize( maxwidth+10, maxheight+10);
  page2->setFixedSize( maxwidth+10, maxheight+10);

  addTab( page1,i18n("General") );
  addTab( page2,i18n("Channels") );

  setCaption(i18n("KMix Preferences") );
}




void Preferences::createChannelConfWindow()
{
  static bool created = false;

  grpbox2a = new QGroupBox (i18n("Mixer channel setup"),page2);
  QLabel *qlb,*qlbd;

  const int entryWidth = 100;
  int ypos=20;
  int x1=10, x2, x3;
  qlbd = new QLabel(grpbox2a);
  qlbd->setText(i18n("Device"));
  qlbd->setFixedWidth(entryWidth);
  qlbd->move(x1,ypos);

  x2 = x1 + qlbd->width() + 4;
  qlb = new QLabel(grpbox2a);
  qlb->setText(i18n("Show"));
  qlb->setFixedWidth(qlb->sizeHint().width());
  qlb->move(x2,ypos);

  x3= x2 + qlb->width() + 8;
  qlb = new QLabel(grpbox2a);
  qlb->setText(i18n("Split"));
  qlb->setFixedWidth(qlb->sizeHint().width());
  qlb->move(x3,ypos);

  int maxwidth = x3 + qlb->width();
  ypos += qlbd->height();


  QPalette qpl (palette());
  qpl.setDisabled( qpl.normal() );

  // Traverse all mix channels and create one line per channel
  for  (MixDevice *mdev = mix->First ; mdev ;  mdev = mdev->Next  ) {
    // 1. line edit
#if 0
    QLabel *qlb;
    qlb = new  QLabel(grpbox2a,mdev->devname);
    qlb->setText(mdev->devname);
    qlb->move(x1,ypos);
    qlb->setFixedWidth(entryWidth);
#endif

    QLineEdit *qle;
    qle = new QLineEdit(grpbox2a,mdev->devname);
    qle->setPalette(qpl);  // Use a palette, where one can read the text
    qle->setText(mdev->devname);
    qle->setEnabled(false);
    qle->move(x1,ypos);
    qle->setFixedWidth(entryWidth);

    // 2. check box  (Show)
    QCheckBox *qcb = new QCheckBox(grpbox2a);
    qcb->setFixedSize(qcb->sizeHint());
    qcb->move(x2,ypos);
    if (mdev->is_disabled)
      qcb->setChecked(false);
    else
      qcb->setChecked(true);

    // 3. check box  (Split)
    QCheckBox *qcbSplit;
    if (mdev->is_stereo) {
      qcbSplit = new QCheckBox(grpbox2a);
      qcbSplit->setFixedSize(qcbSplit->sizeHint());
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

  grpbox2a->setGeometry( 10, 10, maxwidth+10, ypos+10);

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

void Preferences::options2current()
{
  MixSet *cms = mix->TheMixSets->first();
  MixDevice *mdev = mix->First;
  for (ChannelSetup *chanSet = cSetup.first() ; chanSet!=0; chanSet = cSetup.next() ) {

    MixSetEntry *mse;
    for (mse = cms->first();
	 (mse != NULL) && (mse->devnum != chanSet->num) ;
	 mse=cms->next() );

    if (mse == NULL)
      continue;  // entry not found

    else {
      if (mdev->is_stereo)
	mse->StereoLink = ! chanSet->qcbSplit->isChecked();
      mse->is_disabled = ! chanSet->qcbShow->isChecked();
    }
    mdev = mdev->Next;
  }
}

void Preferences::slotApply()
{
  options2current();
  emit optionsApply();
}

void Preferences::slotCancel()
{
  hide();
}

