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
#include "gui/kmixerwidget.h"
#include "gui/kmixtoolbox.h"
#include "gui/mixdevicewidget.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "viewsliders.h"


/**
   This widget is embedded in the KMix Main window. Each Hardware Card is visualized by one KMixerWidget.
   KMixerWidget contains
   a TabBar with n Tabs (at least one per soundcard). These contain View's with sliders, switches and other GUI elements visualizing the Mixer)
*/
KMixerWidget::KMixerWidget( Mixer *mixer,
                            QWidget * parent, ViewBase::ViewFlags vflags, GUIProfile* guiprof,
                            KActionCollection* actionCollection )
   : QWidget( parent ), _mixer(mixer),
     m_topLayout(0), _guiprof(guiprof),
     _actionCollection(actionCollection)
{
	_mainWindow = parent;
	//kDebug() << "kmixWindow created: parent=" << parent << ", parentWidget()=" << parentWidget();
   if ( _mixer )
   {
      createLayout(vflags);
   }
   else
   {
      // No mixer found
      // This is normally never shown. Only if the application
      // creates an invalid KMixerWidget (but this would actually be
      // a programming error).
      QBoxLayout *layout = new QHBoxLayout( this );
      QString s = i18n("Invalid mixer");
      QLabel *errorLabel = new QLabel( s, this );
      errorLabel->setAlignment( Qt::AlignCenter );
      errorLabel->setWordWrap( true );
      layout->addWidget( errorLabel );
   }
}

KMixerWidget::~KMixerWidget()
{
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
   ViewSliders* view = new ViewSliders( this, _guiprof->getId().toLatin1(), _mixer, vflags, _guiprof, _actionCollection );
   bool added = possiblyAddView(view);
   show();
   //    kDebug(67100) << "KMixerWidget::createLayout(): EXIT\n";
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
      vbase->createDeviceWidgets();
      m_topLayout->addWidget(vbase);
      _views.push_back(vbase);
      connect( vbase, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
      // *this will be deleted on rebuildGUI(), so lets queue the signal
      connect( vbase, SIGNAL(rebuildGUI())   , parentWidget(), SLOT(recreateGUIwithSavingView()), Qt::QueuedConnection );
      //connect( vbase, SIGNAL(redrawMixer(QString)), parentWidget(), SLOT(redrawMixer(QString)), Qt::QueuedConnection );

      kDebug() << "CONNECT ViewBase count " << vbase->getMixers().size();
	  foreach ( Mixer* mixer, vbase->getMixers() )
	  {
	    kDebug(67100) << "CONNECT ViewBase controlschanged" << mixer->id();
	   connect ( mixer, SIGNAL(controlChanged()), this, SLOT(refreshVolumeLevelsToplevel()) );
	   connect ( mixer, SIGNAL(controlsReconfigured(QString)), this, SLOT(controlsReconfiguredToplevel(QString)) );
	  }
      return true;
   }
}

void KMixerWidget::controlsReconfiguredToplevel(QString mixerId)
{
	foreach ( ViewBase* vbase, _views)
	{
		vbase->controlsReconfigured(mixerId);
	}
	KMixWindow* kmixWindow = qobject_cast<KMixWindow*>(_mainWindow);
	kDebug() << "kmixWindow to redraw: " << kmixWindow << ", not-casted=" << _mainWindow;
	if (kmixWindow != 0)
	{
		kmixWindow->redrawMixer(mixerId);
	}
}

void KMixerWidget::refreshVolumeLevelsToplevel()
{
	foreach ( ViewBase* vbase, _views)
	{
		vbase->refreshVolumeLevels();
	}
}


/**
 * Returns the current View. Normally we have only one View, so we always return the first view.
 * This method is only here for one reason: We can plug in an action in the main menu, so that
 * 99% of all users will be well served. Those who hack their own XML Profile to contain more than one view
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

void KMixerWidget::setLabels( bool on )
{
    const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
    for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
        ViewBase* mixerWidget = *it;
        mixerWidget->setLabels(on);
    } // for all tabs
//    }
}

void KMixerWidget::setTicks( bool on )
{
    const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
    for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
        ViewBase* mixerWidget = *it;
        mixerWidget->setTicks(on);
    } // for all tabs
}


/**
 */
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

/*
// in RTL mode, the slider is reversed, we cannot just connect the signal to setBalance()
// hack around it before calling _mixer->setBalance()
void KMixerWidget::balanceChanged(int balance)
{
    if (QApplication::isRightToLeft())
        balance = -balance;

    _mixer->setBalance( balance );
}
*/

#include "kmixerwidget.moc"
