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
//  * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "viewbase.h"

// QT
#include <qcursor.h>
#include <QMouseEvent>

// KDE
#include <kaction.h>
#include <kmenu.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kmessagebox.h>
// KMix
#include "dialogviewconfiguration.h"
#include "gui/guiprofile.h"
#include "gui/kmixtoolbox.h"
#include "gui/mixdevicewidget.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"


ViewBase::ViewBase(QWidget* parent, const char* id, Mixer* mixer, Qt::WFlags f, ViewBase::ViewFlags vflags, GUIProfile *guiprof, KActionCollection *actionColletion)
    : QWidget(parent, f), _popMenu(NULL), _actions(actionColletion), _vflags(vflags), _guiprof(guiprof)
{
   setObjectName(id);
   m_viewId = id;
   _mixer = mixer;
   _mixSet = new MixSet();

   // This must be populated now otherwise bad things happen (circular dependancies etc
   // This is due to the fact that setMixSet() calls isDynamic() which in turn needs a populated
   // _mixers array to ensure that this is the case....
   _mixers.insert(_mixer);

   if ( _actions == 0 ) {
      // We create our own action collection, if the actionColletion was 0.
      // This is currently done for the ViewDockAreaPopup, but only because it has not been converted to use the app-wide
      // actionCollection(). This is a @todo.
      _actions = new KActionCollection( this );
   }
   _localActionColletion = new KActionCollection( this );

   // Plug in the "showMenubar" action, if the caller wants it. Typically this is only necessary for views in the KMix main window.
   if ( vflags & ViewBase::HasMenuBar ) {
      KToggleAction *m = static_cast<KToggleAction*>(  _actions->action( name(KStandardAction::ShowMenubar) ) ) ;

      //static_cast<KToggleAction*>(KStandardAction::showMenubar( this, SLOT(toggleMenuBarSlot()), _actions ));
      //_actions->addAction( m->objectName(), m );
      if ( m != 0 ) {
         if ( vflags & ViewBase::MenuBarVisible ) {
            m->setChecked(true);
         }
         else {
            m->setChecked(false);
         }
      }
   }
   if ( !isDynamic() ) {
      QAction *action = _localActionColletion->addAction("toggle_channels");
      action->setText(i18n("&Channels"));
      connect(action, SIGNAL(triggered(bool)), SLOT(configureView()));
   }
/*   connect ( _mixer, SIGNAL(controlChanged()), this, SLOT(refreshVolumeLevels()) );
   connect ( _mixer, SIGNAL(controlsReconfigured(QString)), this, SLOT(controlsReconfigured(QString)) );*/
}

ViewBase::~ViewBase() {
    delete _mixSet;
    // Hint: The GUI profile will not be removed, as it is pooled and might be applied to a new View.
}


void ViewBase::configurationUpdate() {
}

QString ViewBase::id() const {
    return m_viewId;
}

bool ViewBase::isValid() const
{
   return ( _mixSet->count() > 0 || isDynamic() );
}

void ViewBase::setIcons (bool on) { KMixToolBox::setIcons (_mdws, on ); }
void ViewBase::setLabels(bool on) { KMixToolBox::setLabels(_mdws, on ); }
void ViewBase::setTicks (bool on) { KMixToolBox::setTicks (_mdws, on ); }

/**
 * Create all widgets.
 * This is a loop over all supported devices of the corresponding view.
 * On each device add() is called - the derived class must implement add() for creating and placing
 * the real MixDeviceWidget.
 * The added MixDeviceWidget is appended to the _mdws list.
 */
void ViewBase::createDeviceWidgets()
{
    // create devices
    foreach ( MixDevice* md, *_mixSet ) 
    {
        QWidget* mdw = add(md); // a) Let the View implementation do its work
        _mdws.append(mdw); // b) Add it to the local list
    }
    // allow view to "polish" itself
    constructionFinished();

// Moved the following up one Level to KMixerWidget
//   kDebug() << "CONNECT ViewBase count " << _mixers.size();
//  foreach ( Mixer* mixer, _mixers )
//  {
//    kDebug(67100) << "CONNECT ViewBase controlschanged" << mixer->id();
//   connect ( mixer, SIGNAL(controlChanged()), this, SLOT(refreshVolumeLevels()) );
//   connect ( mixer, SIGNAL(controlsReconfigured(QString)), this, SLOT(controlsReconfigured(QString)) );
//  }

    
}

/**
 * Rebuild the View from the (existing) Profile.
 */
void ViewBase::rebuildFromProfile()
{
   emit rebuildGUI();
}


// ---------- Popup stuff START ---------------------
void ViewBase::mousePressEvent( QMouseEvent *e )
{
   if ( e->button() == Qt::RightButton )
      showContextMenu();
}

/**
 * Return a popup menu. This contains basic entries.
 * More can be added by the caller.
 */
KMenu* ViewBase::getPopup()
{
   popupReset();
   return _popMenu;
}

void ViewBase::popupReset()
{
    QAction *act;

    delete _popMenu;
    _popMenu = new KMenu( this );
    _popMenu->addTitle( KIcon( QLatin1String(  "kmix" ) ), i18n("Device Settings" ));

    act = _localActionColletion->action( "toggle_channels" );
    if ( act ) _popMenu->addAction(act);

    act = _actions->action( "options_show_menubar" );
    if ( act ) _popMenu->addAction(act);
}


/**
   This will only get executed, when the user has removed all items from the view.
   Don't remove this method, because then the user cannot get a menu for getting his
   channels back
*/
void ViewBase::showContextMenu()
{
    //kDebug(67100) << "ViewBase::showContextMenu()";
    popupReset();

    QPoint pos = QCursor::pos();
    _popMenu->popup( pos );
}

void ViewBase::controlsReconfigured( const QString& mixer_ID )
{
	// TODO Search _mixers for the correct Mixer*. After that, remove _mixer instance variable
	bool isRelevantMixer = (_mixer->id() == mixer_ID );
	//    if (!isRelevantMixer)
	//    {
	//    	foreach ( Mixer* mixer , _mixers)
	//   		{
	//    		if ( mixer->id() == mixer_ID )
	//    		{
	//    			isRelevantMixer = true;
	//    			break;
	//    		}
	//   		}
	//    }

	if (isRelevantMixer)
	{
		kDebug(67100) << "ViewBase::controlsReconfigured() " << mixer_ID << " is being redrawn (mixset contains: " << _mixSet->count() << ")";
		setMixSet();
		kDebug(67100) << "ViewBase::controlsReconfigured() " << mixer_ID << ": Recreating widgets (mixset contains: " << _mixSet->count() << ")";
		createDeviceWidgets();
	}
}

void ViewBase::refreshVolumeLevels()
{
    // is virtual
}
/**
 * Check all Mixer instances of this view.
 * If at least on is dynamic, return true.
 * Please note that usually there is only one Mixer instance per View.
 * The only exception as of today (June 2011) is the Tray Popup, which
 * can contain controls from e.g. ALSA and  MPRIS2 backends.
 */
bool ViewBase::isDynamic() const
{
  foreach (Mixer* mixer , _mixers )
  {
    if ( mixer->isDynamic() )
      return true;
  }
  return false;
}

void ViewBase::setMixSet()
{
    if ( isDynamic() ) {
        // We need to delete the current MixDeviceWidgets so we can redraw them
        while (!_mdws.isEmpty())
        	delete _mdws.takeFirst();

        _mixSet->clear(); // Clean up our _mixSet so we can reapply our GUIProfile
    }
    _setMixSet();
    
    _mixers.clear();
    _mixers.insert(_mixer);
    foreach ( MixDevice* md, *_mixSet )
    {
//      kDebug() << "VVV Add to " << md->mixer()->id();
//      MixDeviceWidget* mdw = qobject_cast<MixDeviceWidget*>(qw);
      _mixers.insert(md->mixer());
    }
}

int ViewBase::visibleControls()
{
	int visibleCount = 0;
	foreach (QWidget* qw, _mdws)
	{
		if (qw->isVisible())
			++ visibleCount;
	}
	return visibleCount;
}

/**
 * Open the View configuration dialog. The user can select which channels he wants
 * to see and which not.
 */
void ViewBase::configureView() {

    Q_ASSERT( !isDynamic() );
    
    DialogViewConfiguration* dvc = new DialogViewConfiguration(0, *this);
    dvc->show();
    // !! The dialog is modal. Does it delete itself?
}

void ViewBase::toggleMenuBarSlot() {
    //kDebug(67100) << "ViewBase::toggleMenuBarSlot() start\n";
    emit toggleMenuBar();
    //kDebug(67100) << "ViewBase::toggleMenuBarSlot() done\n";
}



void ViewBase::load(KConfig *config)
{
   ViewBase *view = this;
   QString grp = "View.";
   grp += view->id();
   //KConfigGroup cg = config->group( grp );
   kDebug(67100) << "KMixToolBox::loadView() grp=" << grp.toAscii();

   static char guiComplexity[3][20] = { "simple", "extended", "all" };

   // Certain bits are not saved for dynamic mixers (e.g. PulseAudio)
   bool dynamic = isDynamic();

   for ( int tries = 0; tries < 3; tries++ )
   {
   bool atLeastOneControlIsShown = false;
   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         /* Workaround for a bug. KMix in KDE4.0 wrote group names like "[View.Base.Base.Front:0]", with
          a duplicated "Base" which *should* have been Soundcard ID,like in "[View.Base.ALSA::HDA_NVidia:1.Front:0]".

           Workaround: If found, write back correct group name.
        */
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         MixDevice* md = mdw->mixDevice();

         QString devgrp = QString("%1.%2.%3").arg(grp).arg(md->mixer()->id()).arg(md->id());
         KConfigGroup devcg  = config->group( devgrp );

         QString buggyDevgrp = QString("%1.%2.%3").arg(grp).arg(view->id()).arg(md->id());
         KConfigGroup buggyDevgrpCG = config->group( buggyDevgrp );
         if ( buggyDevgrpCG.exists() ) {
            buggyDevgrpCG.copyTo(&devcg);
            // Group will be deleted in KMixerWidget::fixConfigAfterRead()
         }

         if ( mdw->inherits("MDWSlider") )
         {
            // only sliders have the ability to split apart in mutliple channels
            bool splitChannels = devcg.readEntry("Split", !mdw->isStereoLinked());
            mdw->setStereoLinked( !splitChannels );
         }

         bool mdwEnabled = false;
         if ( !dynamic && devcg.hasKey("Show") )
         {
            mdwEnabled = ( true == devcg.readEntry("Show", true) );
        //kDebug() << "Load devgrp" << devgrp << "show=" << mdwEnabled;
            //kDebug(67100) << "KMixToolBox::loadView() for" << devgrp << "from config-file: mdwEnabled==" << mdwEnabled;
         }
         else
         {
            // if not configured in config file, use the default from the profile
             //GUIProfile::ControlSet cset = (view->guiProfile()->getControls());
             foreach ( ProfControl* pControl, view->guiProfile()->getControls() )
             {
                //ProfControl* pControl = *it;
                QRegExp idRegExp(pControl->id);
                //kDebug(67100) << "KMixToolBox::loadView() try match " << (*pControl).id << " for " << mdw->mixDevice()->id();
                if ( mdw->mixDevice()->id().contains(idRegExp) ) {
                   if ( pControl->show == guiComplexity[tries] )
                   {
                      mdwEnabled = true;
                      atLeastOneControlIsShown = true;
                      //kDebug(67100) << "KMixToolBox::loadView() for" << devgrp << "from profile: mdwEnabled==" << mdwEnabled;
                   }
                   break;
                }
             } // iterate over all ProfControl entries
         }
         //mdw->setEnabled(mdwEnabled);  // I have no idea why dialogselectmaster works with "enabled" instead of "visible"
         if (!mdwEnabled) { mdw->hide(); } else { mdw->show(); }

      } // inherits MixDeviceWidget
   } // for all MDW's
   if ( atLeastOneControlIsShown ) {
      break;   // If there were controls in this complexity level, don't try more
   }
   } // for try = 0 ... 1         //kDebug(67100) << "KMixToolBox::loadView() for" << devgrp << "FINAL: mdwEnabled==" << mdwEnabled;
}


/*
 * Saves the View configuration
 */
void ViewBase::save(KConfig *config)
{
   ViewBase *view = this;
   QString grp = "View.";
   grp += view->id();
//   KConfigGroup cg = config->group( grp );
   kDebug(67100) << "KMixToolBox::saveView() grp=" << grp;

   // Certain bits are not saved for dynamic mixers (e.g. PulseAudio)
   bool dynamic = isDynamic();

   for (int i=0; i < view->_mdws.count(); ++i ){
      QWidget *qmdw = view->_mdws[i];
      if ( qmdw->inherits("MixDeviceWidget") )
      {
         MixDeviceWidget* mdw = (MixDeviceWidget*)qmdw;
         MixDevice* md = mdw->mixDevice();

         //kDebug(67100) << "  grp=" << grp.toAscii();
         //kDebug(67100) << "  mixer=" << view->id().toAscii();
         //kDebug(67100) << "  mdwPK=" << mdw->mixDevice()->id().toAscii();

         QString devgrp = QString("%1.%2.%3").arg(grp).arg(md->mixer()->id()).arg(md->id());
         KConfigGroup devcg = config->group( devgrp );

         if ( mdw->inherits("MDWSlider") )
         {
            // only sliders have the ability to split apart in mutliple channels
            devcg.writeEntry( "Split", ! mdw->isStereoLinked() );
         }
         if ( !dynamic ) {
            devcg.writeEntry( "Show" , mdw->isVisibleTo(view) );
            kDebug() << "Save devgrp" << devgrp << "show=" << mdw->isVisibleTo(view);
         }

      } // inherits MixDeviceWidget
   } // for all MDW's

   if ( !dynamic ) {
        // We do not save GUIProfiles (as they cannot be customised) for dynamic mixers (e.g. PulseAudio)
        kDebug(67100) << "GUIProfile is dirty: " << guiProfile()->isDirty();
        if ( guiProfile()->isDirty() )
            guiProfile()->writeProfile();
   }
}


// ---------- Popup stuff END ---------------------

#include "viewbase.moc"
