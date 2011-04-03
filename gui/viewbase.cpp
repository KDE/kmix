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
    : QWidget(parent, f), _actions(actionColletion), _vflags(vflags), _guiprof(guiprof)
{
   setObjectName(id);
   m_viewId = id;
   _mixer = mixer;
   _mixSet = new MixSet();

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
   if ( !_mixer->dynamic() ) {
      QAction *action = _localActionColletion->addAction("toggle_channels");
      action->setText(i18n("&Channels"));
      connect(action, SIGNAL(triggered(bool) ), SLOT(configureView()));
   }
   connect ( _mixer, SIGNAL(controlChanged()), this, SLOT(refreshVolumeLevels()) );
   connect ( _mixer, SIGNAL(controlsReconfigured(const QString&)), this, SLOT(controlsReconfigured(const QString&)) );
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
   return ( _mixSet->count() > 0 || _mixer->dynamic() );
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
    for ( int i=0; i<_mixSet->count(); i++ )
    {
        MixDevice *mixDevice;
        mixDevice = (*_mixSet)[i];
        QWidget* mdw = add(mixDevice);
        _mdws.append(mdw);
    }
    // allow view to "polish" itself
    constructionFinished();
}

/**
 * Rebuild the View from the (existing) Profile.
 */
void ViewBase::rebuildFromProfile()
{
   emit rebuildGUI();
/*
   // Redo everything from scratch, as visibility and the order of the controls might have changed.

   // As the order of the controls is stored in the profile, we need
   // to rebuild the _mixSet 
kDebug() << "rebuild 1";
   _mixSet->clear();
kDebug() << "rebuild 2";
   _mdws.clear();
kDebug() << "rebuild 3";
   setMixSet();
kDebug() << "rebuild 4";
   createDeviceWidgets();
kDebug() << "rebuild 5";
   constructionFinished();
kDebug() << "rebuild 6";
*/
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
    QAction *a;

    _popMenu = new KMenu( this );
    _popMenu->addTitle( KIcon( QLatin1String(  "kmix" ) ), i18n("Device Settings" ));

    a = _localActionColletion->action( "toggle_channels" );
    if ( a ) _popMenu->addAction(a);

    QAction *b = _actions->action( "options_show_menubar" );
    if ( b ) _popMenu->addAction(b);
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
    if ( _mixer->id() == mixer_ID )
    {
        kDebug(67100) << "ViewBase::controlsReconfigured() " << mixer_ID << " is being redrawn (mixset contains: " << _mixSet->count() << ")";
        setMixSet();
        kDebug(67100) << "ViewBase::controlsReconfigured() " << mixer_ID << ": Recreating widgets (mixset contains: " << _mixSet->count() << ")";
        createDeviceWidgets();

        // We've done the low level stuff our selves but let elements
        // above know what has happened so they can reload config etc.
        emit redrawMixer(mixer_ID);
    }
}

void ViewBase::refreshVolumeLevels()
{
    // is virtual
}

Mixer* ViewBase::getMixer()
{
    return _mixer;
}

void ViewBase::setMixSet()
{
    if ( _mixer->dynamic() ) {

        // Check the guiprofile... if it is not the fallback GUIProfile, then
        // make sure that we add a specific entry for any devices not present.
        if ( 0 != _guiprof && GUIProfile::fallbackProfile(_mixer) != _guiprof ) // TODO colin/cesken IMO calling GUIProfile::fallbackProfile(_mixer) is wrong, as it ALWAYS creates a new Object. fallbackProfile() would need to cache the created fallback profiles so this should make any sense.
        {
            kDebug(67100) << "Dynamic mixer " << _mixer->id() << " is NOT using Fallback GUIProfile. Checking to see if new controls are present";

            QList<QString> new_mix_devices;
            MixSet ms = _mixer->getMixSet();
            for (int i=0; i < ms.count(); ++i)
            {
                new_mix_devices.append("^" + ms[i]->id() + "$");
                kDebug(67100) << "new_mix_devices.append => " << ms[i]->id();
            }

            GUIProfile::ControlSet& ctlSet = _guiprof->getControls();

//            std::vector<ProfControl*>::const_iterator itEnd = _guiprof->_controls.end();
//            for ( std::vector<ProfControl*>::const_iterator it = _guiprof->_controls.begin(); it != itEnd; ++it)
//                new_mix_devices.removeAll((*it)->id);
              // TODO Please check this change, Colin
              foreach ( ProfControl* pctl, ctlSet ) {
                  new_mix_devices.removeAll(pctl->id);
              }


            if ( new_mix_devices.count() > 0 ) {
                kDebug(67100) << "Found " << new_mix_devices.count() << " new controls. Adding to GUIProfile";
                QString sctlMatchAll("*");
                while ( new_mix_devices.count() > 0 ) {
                    QString new_mix_devices0 = new_mix_devices.takeAt(0);
                    ctlSet.push_back(new ProfControl(new_mix_devices0, sctlMatchAll));
                }
                _guiprof->setDirty();
            }
        }

        // We need to delete the current MixDeviceWidgets so we can redraw them
        while (!_mdws.isEmpty()) {
            QWidget* mdw = _mdws.last();
            _mdws.pop_back();
            delete mdw;
        }

        // Clean up our _mixSet so we can reapply our GUIProfile
        _mixSet->clear();
    }
    _setMixSet();
}


/**
 * Open the View configuration dialog. The user can select which channels he wants
 * to see and which not.
 */
void ViewBase::configureView() {

    Q_ASSERT( !_mixer->dynamic() );
    
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
         if ( devcg.hasKey("Show") )
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
         devcg.writeEntry( "Show" , mdw->isVisibleTo(view) );
kDebug() << "Save devgrp" << devgrp << "show=" << mdw->isVisibleTo(view);

      } // inherits MixDeviceWidget
   } // for all MDW's

   kDebug(67100) << "GUIProfile is dirty: " << guiProfile()->isDirty();
   if ( guiProfile()->isDirty() ) {
       guiProfile()->writeProfile();
   }
}


// ---------- Popup stuff END ---------------------

#include "viewbase.moc"
