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

#include <qlayout.h>

#include "sets.h"
#include "prefs.h"
#include "prefs.moc"
#include "kmix.h"
#include <klocale.h>

//static char rcsid[]="$Id$";

Preferences::Preferences( QWidget *parent, Mixer *mix ) :
   QTabDialog( parent )
{
  this->mix = mix;

  setCaption(i18n("KMix Preferences") );

  page1 = new QWidget( this );
  page2 = new QWidget( this );

  addTab(page1, i18n("General"));
  addTab(page2, i18n("Channels"));

  // Define page 1
  createUserConfWindow();

  // Define page 2
  createChannelConfWindow();

  setCancelButton(i18n("Cancel"));
  setApplyButton(i18n("Apply"));
  setOkButton(i18n("OK"));


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));

  resize(sizeHint());
}


void Preferences::createUserConfWindow()
{
  QVBoxLayout *topLayout = new QVBoxLayout(page1, 5);

  topLayout->addStretch();

  QButtonGroup *groupBox = new QButtonGroup(i18n("Startup settings"), page1 );
  topLayout->addWidget(groupBox);

  QVBoxLayout *subLayout = new QVBoxLayout(groupBox, 10);
  subLayout->addSpacing(10);

  menubarChk = new QCheckBox(groupBox);
  menubarChk->setText(i18n("Menubar"));
  menubarChk->setMinimumSize(menubarChk->sizeHint());
  subLayout->addWidget(menubarChk);

  tickmarksChk = new QCheckBox(groupBox);
  tickmarksChk->setText(i18n("Tickmarks"));
  tickmarksChk->setMinimumSize(tickmarksChk->sizeHint());
  subLayout->addWidget(tickmarksChk);

  dockingChk = new QCheckBox(groupBox);
  dockingChk->setText(i18n("Allow docking"));
  dockingChk->setMinimumSize(dockingChk->sizeHint());
  subLayout->addWidget(dockingChk);

  topLayout->addStretch();

  subLayout->activate();
  topLayout->activate();
}

void Preferences::createChannelConfWindow()
{
  QVBoxLayout *topLayout = new QVBoxLayout(page2, 5);
  QGroupBox *groupBox = new QGroupBox (i18n("Mixer channel setup"),page2);
  topLayout->addWidget(groupBox);

  QVBoxLayout *subLayout = new QVBoxLayout(groupBox, 10);
  subLayout->addSpacing(10);

  QHBoxLayout *hLayout = new QHBoxLayout();
  subLayout->addLayout(hLayout);

  QLabel *qlb = new QLabel(groupBox);
  qlb->setText(i18n("Device"));
  qlb->setFixedWidth(100);
  qlb->setMinimumHeight(qlb->sizeHint().height());
  hLayout->addWidget(qlb);

  qlb = new QLabel(groupBox);
  qlb->setText(i18n("Show"));
  qlb->setMinimumSize(qlb->sizeHint());
  hLayout->addWidget(qlb);

  qlb = new QLabel(groupBox);
  qlb->setText(i18n("Split"));
  qlb->setMinimumSize(qlb->sizeHint());
  hLayout->addWidget(qlb);

  QPalette qpl (palette());
  qpl.setDisabled( qpl.normal() );

  // Traverse all mix channels and create one line per channel
  for  (MixDevice *mdev = mix->First ; mdev ;  mdev = mdev->Next  ) {
    QHBoxLayout *hLayout = new QHBoxLayout();
    subLayout->addLayout(hLayout);
    // 1. line edit
#if 0
    QLabel *qlb;
    qlb = new  QLabel(groupBox,mdev->devname);
    qlb->setText(mdev->devname);
    qlb->setMinimumSize(qlb->sizeHint());
    hLayout->addWidget(qlb);
#endif

    QLineEdit *qle;
    qle = new QLineEdit(groupBox,mdev->devname);
    qle->setPalette(qpl);  // Use a palette, where one can read the text
    qle->setText(mdev->devname);
    qle->setEnabled(false);
    qle->setFixedWidth(100);
    qle->setMinimumHeight(qle->sizeHint().height());
    hLayout->addWidget(qle);

    // 2. check box  (Show)
    QCheckBox *qcb = new QCheckBox(groupBox);
    qcb->setMinimumSize(qcb->sizeHint());
    if (mdev->is_disabled)
      qcb->setChecked(false);
    else
      qcb->setChecked(true);
    hLayout->addWidget(qcb);

    // 3. check box  (Split)
    QCheckBox *qcbSplit;
    if (mdev->is_stereo) {
      qcbSplit = new QCheckBox(groupBox);
      qcbSplit->setMinimumSize(qcbSplit->sizeHint());
      if (mdev->StereoLink)
	qcbSplit->setChecked(false);
      else
	qcbSplit->setChecked(true);
      hLayout->addWidget(qcbSplit);
    }
    else
      qcbSplit = NULL;

    cSetup.append(new ChannelSetup(mdev->device_num,qle,qcb,qcbSplit));
  }

  subLayout->activate();
  topLayout->activate();
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

