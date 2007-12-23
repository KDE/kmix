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
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>

#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>

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
   QWidget * frame = new QWidget( this );
   frame->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
   
   setMainWidget( frame );
   
   // The _layout will hold two items: The title and the scrollarea
   _layout = new QVBoxLayout(frame );
   
   // --- HEADER ---
   //    kDebug(67100) << "DialogViewConfiguration::DialogViewConfiguration add header" << "\n";
   qlb = new QLabel( i18n("Configuration of the channels."), frame );
   _layout->addWidget(qlb);
   
   QScrollArea* scrollArea = new QScrollArea(frame);
   scrollArea->setWidgetResizable(true); // avoid unnecesary scrollbars
   scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   _layout->addSpacing(KDialog::spacingHint());
   _layout->addWidget(scrollArea);
   
   vboxForScrollView = new QWidget();
   vboxForScrollView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
   QGridLayout* grid = new QGridLayout(vboxForScrollView);
   grid->setHorizontalSpacing(KDialog::spacingHint());
   grid->setVerticalSpacing(0);
   scrollArea->setWidget(vboxForScrollView);

   qlb = new QLabel( i18n("Show/Hide") );
   grid->addWidget(qlb,0,0);
   qlb = new QLabel( i18n("Split"), vboxForScrollView );
   grid->addWidget(qlb,0,1);
   /*
   qlb = new QLabel( i18n("Limit"), vboxForScrollView );
   grid->addWidget(qlb,0,2);
   */
   int i;
    // --- CONTROLS IN THE GRID --- 
   QPalette::ColorRole bgRole;
   for ( i=0; i<mdws.count(); ++i ) {
       if ( i%2 == 0) bgRole = QPalette::Base; else bgRole = QPalette::AlternateBase;
      QWidget *qw = mdws[i];
      if ( qw->inherits("MixDeviceWidget") ) {
            MixDeviceWidget *mdw = static_cast<MixDeviceWidget*>(qw);
            MixDevice *md = mdw->mixDevice();
            QString mdName = md->readableName();
            mdName.replace('&', "&&"); // Quoting the '&' needed, to prevent QCheckBox creating an accelerator
            QCheckBox* cb = new QCheckBox( mdName, vboxForScrollView ); // enable/disable
            cb->setBackgroundRole(bgRole);
            cb->setAutoFillBackground(true);
            _qEnabledCB.append(cb);
            cb->setChecked( mdw->isVisible() );
            grid->addWidget(cb,1+i,0);

            if ( ! md->isEnum() && ( ( md->playbackVolume().count() > 1) || ( md->captureVolume().count() > 1) ) ) {
                cb = new QCheckBox( "", vboxForScrollView ); // split
                cb->setBackgroundRole(bgRole);
                cb->setAutoFillBackground(true);
                _qSplitCB.append(cb);
                cb->setChecked( ! mdw->isStereoLinked() );
                grid->addWidget(cb,1+i,1);
            }
            else {
                _qSplitCB.append(0);
            }
            /*
            if ( ! md->isEnum() && ( md->playbackVolume().count() + md->captureVolume().count() >0 ) ) {
                cb = new QCheckBox( "", vboxForScrollView ); // limit
                cb->setBackgroundRole(bgRole);
                cb->setAutoFillBackground(true);
                _qLimitCB.append(cb);
                grid->addWidget(cb,1+i,2);
            }
            else {
            */
                _qLimitCB.append(0);
            /*}*/
      }
   } // for all MDW's

   scrollArea->updateGeometry();
   updateGeometry();
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
        MixDeviceWidget *mdw;
      
        if ( qw->inherits("MixDeviceWidget") ) {
            mdw = static_cast<MixDeviceWidget*>(qw);
        }
        else {
            continue;
        }
       
        QCheckBox *cb = _qEnabledCB[i];
        if ( cb != 0 ) {
            mdw->setVisible( cb->isChecked());
        } // show-hide

        cb = _qSplitCB[i];
        if ( cb != 0 ) {
            mdw->setStereoLinked( ! cb->isChecked() );
        } // split
        
   } // for all MDW's

   // --- Step 2: Tell the view, that it has changed (probably it needs some "polishing" ---
   _view.configurationUpdate();
}


QSize DialogViewConfiguration::sizeHint() const {
    QSize size = vboxForScrollView->sizeHint();
    // The +50 is a workaround, because KDialog does not handle our case correctly (using a QScrollarea)
    size.rwidth() += 50;
    size.rheight() += KDialog::spacingHint();
    size.rheight() += qlb->height();
    return size;
}

#include "dialogviewconfiguration.moc"

