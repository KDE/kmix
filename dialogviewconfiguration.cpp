/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QCheckBox>
#include <QLayout>
#include <QLabel>

#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>

#include "dialogviewconfiguration.h"
#include "mixdevicewidget.h"
#include "mixdevice.h"


DialogViewConfiguration::DialogViewConfiguration( QWidget*, ViewBase& view)
    : KDialog(  0),
      _view(view)
{
    setCaption( i18n( "Configure" ) );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    QList<QWidget *> &mdws = view._mdws;
    QFrame * frame = new QFrame( this );
    setMainWidget( frame );
    _layout = new QVBoxLayout(frame );
    _layout->setObjectName( "_layout" );

    //    kDebug(67100) << "DialogViewConfiguration::DialogViewConfiguration add header" << "\n";
    QLabel* qlb = new QLabel( i18n("Configure"), frame );
    //QLabel* qlb = new QLabel( i18n("Show"), plainPage() );
    _layout->addWidget(qlb);

    for ( int i=0; i<mdws.count(); ++i ) {
	   QWidget *qw = mdws[i];
	if ( qw->inherits("MixDeviceWidget") ) {
	    MixDeviceWidget *mdw = static_cast<MixDeviceWidget*>(qw);
	    QString mdName = mdw->mixDevice()->name();
            mdName.replace('&', "&&"); // Quoting the '&' needed, to prevent QCheckBox creating an accelerator
	    QCheckBox* cb = new QCheckBox( mdName, frame );
	    _qEnabledCB.append(cb);
	    cb->setChecked( !mdw->isDisabled() ); //mdw->isVisible() );
	    _layout->addWidget(cb);
	}
    }
    _layout->activate();
    resize(_layout->sizeHint() );
    connect( this, SIGNAL(okClicked())   , this, SLOT(apply()) );
}

DialogViewConfiguration::~DialogViewConfiguration()
{
}

void DialogViewConfiguration::apply()
{
    QList<QWidget *> &mdws = _view._mdws;

    // --- 2-Step Apply ---

    // --- Step 1: Show and Hide Widgets ---
    for ( int i=0; i<mdws.count(); ++i ) {
	   QWidget *qw = mdws[i];
		QCheckBox *cb = _qEnabledCB[i];
	if ( qw->inherits("MixDeviceWidget") ) {
	    MixDeviceWidget *mdw = static_cast<MixDeviceWidget*>(qw);
	    if ( cb->isChecked() ) {
		mdw->setDisabled(false);
	    }
	    else {
		mdw->setDisabled(true);
	    }

	}
    }

    // --- Step 2: Tell the view, that it has changed (probably it needs some "polishing" ---
    _view.configurationUpdate();
}

QSize DialogViewConfiguration::sizeHint() const {
    //    kDebug(67100) << "DialogViewConfiguration::sizeHint() is (100,500)\n";
    return _layout->sizeHint();
}

#include "dialogviewconfiguration.moc"

