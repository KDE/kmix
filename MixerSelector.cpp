/*
 * KMix -- KDE's full featured mini mixer
 *
 * $Id$
 *
 * MixerSelector
 * Copyright (C) 2003 Christian Esken <esken@kde.org>
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

#include "MixerSelector.h"
#include "MixerSelectionInfo.h"
#include "mixer.h"

#include <qcombobox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qbutton.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qvbox.h>
#include <qhbox.h>

#include <kdialogbase.h>
#include <klocale.h>

//#include <kdebug.h>

/**
 * Mixer selection dialog. Used for inserting a new mixer.
 * Allows to select mixer, set the mixer name, whether to distribute
 * channels on Tabs and (in next version) to which devices to show
 * (will by implemented by allowing to specify a device category).
 * 
 * Use the construtor for constructing the dialog, use exec() to show it.
 */
MixerSelector::MixerSelector(QPtrList<Mixer> &mixers, QWidget * parent, const char * name, WFlags f) :
	QWidget(parent,name,f)
{
   int n = 1;
   QStringList lst;
   for (Mixer *mixer=mixers.first(); mixer; mixer=mixers.next())
   {
      QString s;
      s.sprintf("%i. %s", n, mixer->mixerName().ascii());
      lst << s;
      n++;
   }

	dialog =
		new KDialogBase(this,"Select Mixer",true,i18n("Select Mixer"),  KDialogBase::Ok|KDialogBase::Cancel);

	vbox = new QVBox( dialog, "mixselector" );
	dialog->setMainWidget(vbox);
	hwNames = new QComboBox( false, vbox);
	hwNames->insertStringList(lst);
	
	QHBox *hbox = new QHBox( vbox );
	/*QLabel *nameLabel =*/ new QLabel(i18n("Name:"), hbox);
	shownName = new QLineEdit(hbox);

	connect (hwNames, SIGNAL(highlighted(const QString&)), this, SLOT(newMixerSelected(const QString&)) );
	newMixerSelected( hwNames->currentText() );
	
	distributeCheck = new QCheckBox(i18n("&Distribution to multiple tabs allowed"), vbox);
	distributeCheck->setChecked(true);

	dialog->setMinimumSize( 300, 180);
}

MixerSelector::~MixerSelector()
{
	// no need to delete anything (widgets will be destroyed by Qt).
}

/**
 * Executes the dialog and returns the values.
 * @returns A MixerSelectionInfo instance with the selected value. You
 *  have to delete this object after using its content.
 *  If the user pressed the Cancel button, 0 will be returned.
 */
MixerSelectionInfo* MixerSelector::exec() {
	/*
	connect(dialog, SIGNAL(okClicked()), this, okClicked());
	connect(dialog, SIGNAL(cancelClicked()), this, cancelClicked());
	*/
	int ret = dialog->exec();
	if (ret != QDialog::Accepted) {
		return 0;
	} else {
		MixerSelectionInfo *msi = new MixerSelectionInfo(hwNames->currentItem(), shownName->text(), distributeCheck->isChecked(), MixDevice::ALL);
		return msi;
	}
}

void MixerSelector::newMixerSelected(const QString &newMixer) {
	// using mid(3) here to strip off the numbering prefix (e.g. "1. AU8830 Soundcard")
	shownName->setText(newMixer.mid(3));
}


#include "MixerSelector.moc"

