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
#include <qlayout.h>
#include <qglobal.h>

//static char rcsid[]="$Id$";

Preferences::Preferences( QWidget *parent, Mixer *mix ) :
   QTabDialog( parent )
{
  this->mix = mix;

  page1 = new QWidget( this );
  page2 = new QWidget( this );

  // Define page 1
  createOptionsConfWindow(page1);

  // Define page 2
  createChannelConfWindow(page2);

  setCancelButton(i18n("&Cancel"));
  setApplyButton(i18n("&Apply"));
  setOkButton(i18n("&OK"));


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));


  addTab( page1,i18n("&General") );
  addTab( page2,i18n("C&hannels") );

  setCaption(i18n("KMix Preferences") );
}


void Preferences::createOptionsConfWindow(QWidget *p)
{
  QBoxLayout *top = new QVBoxLayout(p, 10);
  QButtonGroup *grpbox = new QButtonGroup(i18n("Startup settings"), p );
  top->addWidget(grpbox);

  QBoxLayout *l = new QVBoxLayout(grpbox, 5);
  l->addSpacing(10);

  menubarChk = new QCheckBox(grpbox);
  menubarChk->setText(i18n("&Menubar"));
  l->addWidget(menubarChk);

  tickmarksChk = new QCheckBox(grpbox);
  tickmarksChk->setText(i18n("&Tickmarks"));
  l->addWidget(tickmarksChk);
  
  dockingChk = new QCheckBox(grpbox);
  dockingChk->setText(i18n("Allow &docking"));
  l->addWidget(dockingChk);
}


void Preferences::createChannelConfWindow(QWidget *p)
{
  static bool created = false;

  QBoxLayout *top = new QHBoxLayout(p, 10);
  QGroupBox *grpbox = new QGroupBox (i18n("Mixer channel setup"), p);
  top->addWidget(grpbox);
  QGridLayout *l = new QGridLayout(grpbox, 1, 3, 5);
  int lay_i = 0; //CT use it to grow the grid
  l->addRowSpacing(lay_i,10);
  l->setRowStretch(lay_i++,0);

  QLabel *qlb;

  qlb = new QLabel(i18n("Device"), grpbox);
  l->addWidget(qlb, lay_i, 0);

  qlb = new QLabel(i18n("Show"), grpbox);
  l->addWidget(qlb, lay_i, 1);

  qlb = new QLabel(i18n("Split"), grpbox);
  l->addWidget(qlb, lay_i, 2);

  l->setRowStretch(lay_i++, 0);
  l->setRowStretch(lay_i++, 1);

  QPalette qpl (palette());
  qpl.setDisabled( qpl.normal() );


  // Traverse all mix channels and create one line per channel
  for  (MixDevice *mdev = mix->First ; mdev ;  mdev = mdev->Next  ) {
    // 1. line edit
    QLineEdit *qle;
#if 0
    qle->setEnabled(false);
    l->addWidget(qle, lay_i, 0); 

    qlb = new  QLabel(mdev->name(), grpbox, mdev->name());
    qlb->setEnabled(false);
    l->addWidget(qlb, lay_i, 0); 
#else

#if QT_VERSION >= 200
    qle = new QLineEdit(mdev->name(), grpbox, mdev->name().ascii());
#else
    qle = new QLineEdit(grpbox, (const char*)(mdev->name()));
#endif
    //qle->setPalette(qpl);  // Use a palette, where one can read the text
    //qle->setEnabled(false);
    l->addWidget(qle, lay_i, 0); 
#endif

    // 2. check box  (Show)
    QCheckBox *qcb = new QCheckBox(grpbox);
    if (mdev->disabled())
      qcb->setChecked(false);
    else
      qcb->setChecked(true);
    l->addWidget(qcb, lay_i, 1); 

    // 3. check box  (Split)
    QCheckBox *qcbSplit;
    if (mdev->stereo()) {
      qcbSplit = new QCheckBox(grpbox);
      if (mdev->stereoLinked() )
	qcbSplit->setChecked(false);
      else
	qcbSplit->setChecked(true);
      l->addWidget(qcbSplit, lay_i, 2);
    }
    else
      qcbSplit = NULL;

    l->setRowStretch(lay_i++, 0);
    l->setRowStretch(lay_i++, 1);

    cSetup.append(new ChannelSetup(mdev->num(),qle,qcb,qcbSplit));
  }

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
      if (mdev->stereo())
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

