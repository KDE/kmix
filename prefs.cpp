/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
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

  // Define page 3

  setCancelButton(i18n("&Cancel"));
  setApplyButton(i18n("&Apply"));
  setOkButton(i18n("&OK"));


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));
  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < mix->size(); l_i_mixDevice++) {
    // We will sync the prefs dialog completely, whenever something changes in the MainWindow
    connect( & ((*mix)[l_i_mixDevice]), SIGNAL(relayout()), this, SLOT(slotUpdatelayout()) );
  }
  

  addTab( page1,i18n("&General") );
  addTab( page2,i18n("C&hannels") );
  showPage(page2); // !!! DEBUG

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


void Preferences::createChannelConfWindow(QWidget *val_qw_parent)
{
  static bool created = false;

  QBoxLayout *top = new QVBoxLayout(val_qw_parent, 10);

  // Add mixer name
  QString l_s_cardTitle;
  l_s_cardTitle = mix->mixerName();
  QLabel *l_qw_tmp = new  QLabel( l_s_cardTitle , val_qw_parent);
  QFont f("Helvetica", 12, QFont::Bold);
  l_qw_tmp->setFont( f );
  top->addWidget(l_qw_tmp);

  // Add set selection Combo Box
  i_combo_setSelect = new QComboBox(val_qw_parent);
  i_combo_setSelect->insertItem("Set selector - This is not functional yet!");

  for ( unsigned int l_i_set=0; l_i_set < mix->i_set_allMixSets->size(); l_i_set++ ) {
    MixSet &l_mixSet =  *((*(mix->i_set_allMixSets))[l_i_set]);
    i_combo_setSelect->insertItem( "bla" );//l_mixSet->name() );
  }
  top->addWidget(i_combo_setSelect);

  QGroupBox *grpbox = new QGroupBox (i18n("Mixer channel setup"), val_qw_parent);
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
  for ( unsigned int l_i_mixDevice = 0; l_i_mixDevice < mix->size(); l_i_mixDevice++) {
    MixDevice &MixPtr = (*mix)[l_i_mixDevice];

    // 1. line edit
    QLineEdit *qle;
    qle = new QLineEdit(MixPtr.name(), grpbox, MixPtr.name().ascii());
    l->addWidget(qle, lay_i, 0); 

    // 2. check box  (Show)
    QCheckBox *qcb = new QCheckBox(grpbox);

#if 0 // remove soon
#warning This will be removed as soon as possible
    if (MixPtr.disabled())
      qcb->setChecked(false);
    else
      qcb->setChecked(true);
#endif

    l->addWidget(qcb, lay_i, 1); 

    // 3. check box  (Split)
    QCheckBox *qcbSplit;
    if (MixPtr.stereo()) {
      qcbSplit = new QCheckBox(grpbox);

#if 0 // remove soon
#warning This will be removed as soon as possible
      if (MixPtr.stereoLinked() )
	qcbSplit->setChecked(false);
      else
	qcbSplit->setChecked(true);
#endif

      l->addWidget(qcbSplit, lay_i, 2);
    }
    else
      qcbSplit = NULL;

    l->setRowStretch(lay_i++, 0);
    l->setRowStretch(lay_i++, 1);

    cSetup.append(new ChannelSetup(MixPtr.num(),qle,qcb,qcbSplit));
  }

  current2options();
  created = true;
}      


/// Called, when the user activates the options dialog (by selecting the menu entry)
void Preferences::slotShow()
{
  show();
}

/// Called, when the user pressed the "OK" button of the configuration dialog
void Preferences::slotOk()
{
  slotApply();
  hide();
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


void Preferences::slotUpdatelayout()
{
  // When the MainWinodw GUI changes (Splitting), it will a signal. This is connected
  // to this slot, so the prefs window can update it's view to match the MainWindow settings.
  current2options();
}


void Preferences::options2current()
{
#warning REDO it completely
#if 0
  MixSet *cms = mix->i_set_current;

  unsigned int l_i_mixDevice = 1;
  for (ChannelSetup *chanSet = cSetup.first() ; chanSet!=0; chanSet = cSetup.next() ) {
    MixDevice &MixPtr = (*mix)[l_i_mixDevice];

    MixSetEntry &mse  = cms;
    for (mse = cms->first();
	 (mse != NULL) && (mse->devnum != chanSet->num) ;
	 mse=cms->next() );

    if (mse == NULL)
      continue;  // entry not found

    else {
      if (MixPtr.stereo()) {
	mse->StereoLink = ! chanSet->qcbSplit->isChecked();
      }
      mse->is_disabled = ! chanSet->qcbShow->isChecked();
    }
    l_i_mixDevice++;
  }
#endif
}


void Preferences::current2options()
{
#warning REDO it completely
#if 0
  // Read from current set;
  MixSet *cms = mix->i_set_current;

  unsigned int l_i_mixDevice = 1;
  for (ChannelSetup *chanSet = cSetup.first() ; chanSet!=0; chanSet = cSetup.next() ) {
    MixDevice &MixPtr = (*mix)[l_i_mixDevice];

    MixSetEntry *mse;
    for (mse = cms->first();
	 (mse != NULL) && (mse->devnum != chanSet->num) ;
	 mse=cms->next() );

    if (mse == NULL)
      continue;  // entry not found

    else {
      if (MixPtr.stereo()) {
	chanSet->qcbSplit->setChecked( ! mse->StereoLink) ;
      }
      chanSet->qcbShow->setChecked( ! mse->is_disabled) ;
    }
    l_i_mixDevice++;
  }
#endif
}
