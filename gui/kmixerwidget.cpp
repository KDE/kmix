/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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

#include "gui/kmixerwidget.h"

// Qt
#include <QLabel>
#include <qpixmap.h>
#include <QString>
#include <qtoolbutton.h>
#include <qapplication.h> // for QApplication::revsreseLayout()
#include <QVBoxLayout>

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>
#include <ktabwidget.h>

// KMix
#include "apps/kmix.h"
#include "gui/guiprofile.h"
#include "gui/kmixtoolbox.h"
#include "gui/mixdevicewidget.h"
#include "gui/viewsliders.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"


/**
   This widget is embedded in the KMix Main window. Each Hardware Card is visualized by one KMixerWidget.
   KMixerWidget contains
   a TabBar with n Tabs (at least one per soundcard). These contain View's with sliders, switches and other GUI elements visualizing the Mixer)
*/
KMixerWidget::KMixerWidget( Mixer *mixer,
                            QWidget * parent, ViewBase::ViewFlags vflags, QString guiprofId,
                            KActionCollection* actionCollection )
   : QWidget( parent ), _mixer(mixer),
     m_topLayout(0), _guiprofId(guiprofId),
     _actionCollection(actionCollection)
{
	createLayout(vflags);
}

KMixerWidget::~KMixerWidget()
{
  foreach (ViewBase *view, _views)
  {
    delete view;
  }
  _views.clear();
}

/**
 * Creates the widgets as described in the KMixerWidget constructor
 */
void KMixerWidget::createLayout(ViewBase::ViewFlags vflags)
{
   // delete old objects
   delete m_topLayout;
  
   // create main layout
   m_topLayout = new QVBoxLayout( this );
   m_topLayout->setSpacing( 3 );
   m_topLayout->setObjectName( QLatin1String( "m_topLayout" ) );

   /*******************************************************************
   *  Now the main GUI is created.
   * 1) Select a (GUI) profile,  which defines  which controls to show on which Tab
   * 2a) Create the Tab's and the corresponding Views
   * 2b) Create device widgets
   * 2c) Add Views to Tab
   ********************************************************************/
   GUIProfile* guiprof = getGuiprof();
   if ( guiprof != 0 )
   {
    kDebug() << "Add a view " << _guiprofId;
    ViewSliders* view = new ViewSliders( this, guiprof->getId(), _mixer, vflags, _guiprofId, _actionCollection );
    possiblyAddView(view);
   }
    show();
}



/**
 * Add the given view, if it is valid - it must have controls or at least have the chance to gain some (dynamic views)
 * @param vbase
 * @return true, if the view was added
 */
bool KMixerWidget::possiblyAddView(ViewBase* vbase)
{
   if ( ! vbase->isValid()  ) {
      delete vbase;
      return false;
   }
   else {
      m_topLayout->addWidget(vbase);
      _views.push_back(vbase);
      connect( vbase, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
      kDebug() << "CONNECT ViewBase count " << vbase->getMixers().size();
      return true;
   }
}

/**
 * Returns the current View. Normally we have only one View, so we always return the first view.
 * This method is only here for one reason: We can plug in an action in the main menu for the view, so that
 * 99,99% of all users will be well served. Those who hack their own XML Profile to contain more than one view
  must  use the context menu for configuring the additional views.
 */
ViewBase* KMixerWidget::currentView()
{
	return _views.empty() ? 0 : _views[0];
}


void KMixerWidget::setIcons( bool on )
{
const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
    for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
        ViewBase* mixerWidget = *it;
        mixerWidget->setIcons(on);
    } // for all tabs
}


void KMixerWidget::loadConfig( KConfig *config )
{
   kDebug(67100) << "KMixerWidget::loadConfig()";
    const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
    for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
        ViewBase* view = *it;
        kDebug(67100) << "KMixerWidget::loadConfig()" << view->id();
        view->load(config);
        view->configurationUpdate();
    } // for all tabs
}



void KMixerWidget::saveConfig( KConfig *config )
{
   kDebug(67100) << "KMixerWidget::saveConfig()";
    const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
    for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
        ViewBase* view = *it;
        kDebug(67100) << "KMixerWidget::saveConfig()" << view->id();
        view->save(config);
    } // for all tabs
}


void KMixerWidget::toggleMenuBarSlot() {
    emit toggleMenuBar();
}

#include "kmixerwidget.moc"
