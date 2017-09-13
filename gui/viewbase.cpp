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
#include <QMenu>
#include <QMouseEvent>

// KDE
#include <klocalizedstring.h>
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
#include "gui/mdwslider.h"
#include "core/ControlManager.h"
#include "core/GlobalConfig.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"


/**
 * Creates an empty View. To populate it with MixDevice instances, you must implement
 * _setMixSet() in your derived class.
 */
ViewBase::ViewBase(QWidget* parent, QString id, Qt::WindowFlags f, ViewBase::ViewFlags vflags, QString guiProfileId, KActionCollection *actionColletion)
    : QWidget(parent, f), _popMenu(NULL), _actions(actionColletion), _vflags(vflags), _guiProfileId(guiProfileId)
, guiLevel(GuiVisibility::GuiSIMPLE)
{
   setObjectName(id);
   // When loding the View from the XML profile, guiLevel can get overridden
   m_viewId = id;
   configureIcon = QIcon::fromTheme( QLatin1String( "configure" ));

   
   if ( _actions == 0 ) {
      // We create our own action collection, if the actionColletion was 0.
      // This is currently done for the ViewDockAreaPopup, but only because it has not been converted to use the app-wide
      // actionCollection(). This is a @todo.
      _actions = new KActionCollection( this );
   }
   _localActionColletion = new KActionCollection( this );

   // Plug in the "showMenubar" action, if the caller wants it. Typically this is only necessary for views in the KMix main window.
   if ( vflags & ViewBase::HasMenuBar )
   {
      KToggleAction *m = static_cast<KToggleAction*>(  _actions->action( name(KStandardAction::ShowMenubar) ) ) ;
      if ( m != 0 ) {
         bool visible = ( vflags & ViewBase::MenuBarVisible );
         m->setChecked(visible);
      }
   }
}

ViewBase::~ViewBase()
{
    // Hint: The GUI profile will not be removed, as it is pooled and might be applied to a new View.
}


void ViewBase::addMixer(Mixer *mixer)
{
  _mixers.append(mixer);
}

//void ViewBase::configurationUpdate() {
//}



QPushButton* ViewBase::createConfigureViewButton()
{
	QPushButton* configureViewButton = new QPushButton(configureIcon, "", this);
	configureViewButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	configureViewButton->setToolTip(i18n( "Configure Channels" ));
	connect(configureViewButton, SIGNAL(clicked(bool)), SLOT(configureView()));
	return configureViewButton;
}

void ViewBase::updateGuiOptions()
{
    setTicks(GlobalConfig::instance().data.showTicks);
    setLabels(GlobalConfig::instance().data.showLabels);
    updateMediaPlaybackIcons();
}

QString ViewBase::id() const {
    return m_viewId;
}

bool ViewBase::isValid() const
{
   return ( !_mixSet.isEmpty() || isDynamic() );
}

void ViewBase::setIcons (bool on) { KMixToolBox::setIcons (_mdws, on ); }
void ViewBase::setLabels(bool on) { KMixToolBox::setLabels(_mdws, on ); }
void ViewBase::setTicks (bool on) { KMixToolBox::setTicks (_mdws, on ); }

/**
 * Updates all playback icons to their (new) state, e.g. Paused, or Playing
 */
void ViewBase::updateMediaPlaybackIcons()
{
	for (int i = 0; i < _mdws.count(); ++i)
	{
		// Currently media controls are always attached to sliders => use MDWSlider
		MDWSlider* mdw = qobject_cast<MDWSlider*>(_mdws[i]);
		if (mdw != 0)
		{
			mdw->updateMediaButton();
		}
	}
}

/**
 * Create all widgets.
 * This is a loop over all supported devices of the corresponding view.
 * On each device add() is called - the derived class must implement add() for creating and placing
 * the real MixDeviceWidget.
 * The added MixDeviceWidget is appended to the _mdws list.
 */
void ViewBase::createDeviceWidgets()
{
  _setMixSet();
    foreach ( shared_ptr<MixDevice> md, _mixSet )
    {
        QWidget* mdw = add(md); // a) Let the View implementation do its work
        _mdws.append(mdw); // b) Add it to the local list
        connect(mdw, SIGNAL(guiVisibilityChange(MixDeviceWidget*, bool)), SLOT(guiVisibilitySlot(MixDeviceWidget*, bool)));
    }

    if ( !isDynamic() )
    {
      QAction *action = _localActionColletion->addAction("toggle_channels");
      action->setText(i18n("&Channels"));
      connect(action, SIGNAL(triggered(bool)), SLOT(configureView()));
   }

        // allow view to "polish" itself
      constructionFinished();
}

/**
 * Called when a specific control is to be shown or hidden. At the moment it is only called via
 * the "hide" action in the MDW context menu.
 *
 * @param mdw
 * @param enable
 */
void ViewBase::guiVisibilitySlot(MixDeviceWidget* mdw, bool enable)
{
	MixDevice* md = mdw->mixDevice().get();
	qCDebug(KMIX_LOG) << "Change " << md->id() << " to visible=" << enable;
	ProfControl* pctl = findMdw(md->id());
	if (pctl == 0)
	{
		qCWarning(KMIX_LOG) << "MixDevice not found, and cannot be hidden, id=" << md->id();
		return; // Ignore
	}

	pctl->setVisible(enable);
	ControlManager::instance().announce(md->mixer()->id(), ControlChangeType::ControlList, QString("ViewBase::guiVisibilitySlot"));
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
QMenu* ViewBase::getPopup()
{
   popupReset();
   return _popMenu;
}

void ViewBase::popupReset()
{
    QAction *act;

    delete _popMenu;
    _popMenu = new QMenu( this );
    _popMenu->addSection( QIcon::fromTheme( QLatin1String(  "kmix" ) ), i18n("Device Settings" ));

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
    //qCDebug(KMIX_LOG) << "ViewBase::showContextMenu()";
    popupReset();

    QPoint pos = QCursor::pos();
    _popMenu->popup( pos );
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

bool ViewBase::pulseaudioPresent() const
{
	// We do not use Mixer::pulseaudioPresent(), as we are only interested in Mixer instances contained in this View.
  foreach (Mixer* mixer , _mixers )
  {
	  if ( mixer->getDriverName() == "PulseAudio" )
		  return true;
  }
  return false;
}


void ViewBase::resetMdws()
{
      // We need to delete the current MixDeviceWidgets so we can redraw them
      while (!_mdws.isEmpty())
	      delete _mdws.takeFirst();

      // _mixSet contains shared_ptr instances, so clear() should be enough to prevent mem leak
      _mixSet.clear(); // Clean up our _mixSet so we can reapply our GUIProfile
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
void ViewBase::configureView()
{
    Q_ASSERT( !isDynamic() );
    Q_ASSERT( !pulseaudioPresent() );
    
    DialogViewConfiguration* dvc = new DialogViewConfiguration(0, *this);
    dvc->show();
}

void ViewBase::toggleMenuBarSlot() {
    //qCDebug(KMIX_LOG) << "ViewBase::toggleMenuBarSlot() start\n";
    emit toggleMenuBar();
    //qCDebug(KMIX_LOG) << "ViewBase::toggleMenuBarSlot() done\n";
}



/**
 * Loads the configuration of this view.
 * <p>
 * Future directions: The view should probably know its config in advance, so we can use it in #load() and #save()
 *
 *
 * @param config The view for this config
 */
void ViewBase::load(KConfig *config)
{
	ViewBase *view = this;
	QString grp = "View.";
	grp += view->id();
	//KConfigGroup cg = config->group( grp );
	qCDebug(KMIX_LOG)
	<< "KMixToolBox::loadView() grp=" << grp.toLatin1();

	static GuiVisibility guiVisibilities[3] =
	{ GuiVisibility::GuiSIMPLE, GuiVisibility::GuiEXTENDED, GuiVisibility::GuiFULL };

	bool guiLevelSet = false;
	for (int i=0; i<3; ++i)
	{
		GuiVisibility& guiCompl = guiVisibilities[i];
		bool atLeastOneControlIsShown = false;
		foreach(QWidget *qmdw, view->_mdws)
		{
			if (qmdw->inherits("MixDeviceWidget"))
			{
				MixDeviceWidget* mdw = (MixDeviceWidget*) qmdw;
				shared_ptr<MixDevice> md = mdw->mixDevice();
				QString devgrp = md->configGroupName(grp);
				KConfigGroup devcg = config->group(devgrp);

				if (mdw->inherits("MDWSlider"))
				{
					// only sliders have the ability to split apart in mutliple channels
					bool splitChannels = devcg.readEntry("Split", !mdw->isStereoLinked());
					mdw->setStereoLinked(!splitChannels);
				}

				// Future directions: "Visibility" is very dirty: It is read from either config file or
				// GUIProfile. Thus we have a lot of doubled mdw visibility code all throughout KMix.
				bool mdwEnabled = false;

				// Consult GuiProfile for visibility
				mdwEnabled = findMdw(mdw->mixDevice()->id(), guiCompl) != 0; // Match GUI complexity
//				qCWarning(KMIX_LOG) << "---------- FIRST RUN: md=" << md->id() << ", guiVisibility=" << guiCompl.getId() << ", enabled=" << mdwEnabled;

				if (mdwEnabled)
				{
					atLeastOneControlIsShown = true;
				}
				mdw->setVisible(mdwEnabled);
			} // inherits MixDeviceWidget
		} // for all MDW's
		if (atLeastOneControlIsShown)
		{
			guiLevelSet  = true;
			setGuiLevel(guiCompl);
			break;   // If there were controls in this complexity level, don't try more
		}
	} // for try = 0 ... 1

	if (!guiLevelSet)
		setGuiLevel(guiVisibilities[2]);
}

void ViewBase::setGuiLevel(GuiVisibility& guiLevel)
{
	this->guiLevel = guiLevel;
}

/**
 * Checks whether the given mixDevice shall be shown according to the requested
 * GuiVisibility. All ProfControl objects are inspected. The first found is returned.
 * 
 * @param mdwId The control ID
 * @param requestedGuiComplexityName The GUI name
 * @return The corresponding ProfControl*
 *
 */
ProfControl* ViewBase::findMdw(const QString& mdwId, GuiVisibility visibility)
{
	foreach ( ProfControl* pControl, guiProfile()->getControls() )
	{
		QRegExp idRegExp(pControl->id);
		if ( mdwId.contains(idRegExp) )
		{
			if (pControl->getVisibility().satisfiesVisibility(visibility))
			{
//				qCDebug(KMIX_LOG) << "  MATCH " << (*pControl).id << " for " << mdwId << " with visibility " << pControl->getVisibility().getId() << " to " << visibility.getId();
				return pControl;
			}
			else
			{
//				qCDebug(KMIX_LOG) << "NOMATCH " << (*pControl).id << " for " << mdwId << " with visibility " << pControl->getVisibility().getId() << " to " << visibility.getId();
			}
		}
	} // iterate over all ProfControl entries

	return 0; // not found
}


/**
 * Returns the ProfControl* to the given id. The first found is returned.
 * GuiVisibilityis not taken into account. . All ProfControl objects are inspected.
 *
 * @param id The control ID
 * @return The corresponding ProfControl*, or 0 if no match was found
 */
ProfControl* ViewBase::findMdw(const QString& id)
{
	foreach ( ProfControl* pControl, guiProfile()->getControls() )
	{
		QRegExp idRegExp(pControl->id);
		//qCDebug(KMIX_LOG) << "KMixToolBox::loadView() try match " << (*pControl).id << " for " << mdw->mixDevice()->id();
		if ( id.contains(idRegExp) )
		{
			return pControl;
		}
	} // iterate over all ProfControl entries

	return 0;// not found
}

/*
 * Saves the View configuration
 */
void ViewBase::save(KConfig *config)
{
	ViewBase *view = this;
	QString grp = "View.";
	grp += view->id();

	// Certain bits are not saved for dynamic mixers (e.g. PulseAudio)
	bool dynamic = isDynamic();  // TODO 11 Dynamic view configuration

	for (int i = 0; i < view->_mdws.count(); ++i)
	{
		QWidget *qmdw = view->_mdws[i];
		if (qmdw->inherits("MixDeviceWidget"))
		{
			MixDeviceWidget* mdw = (MixDeviceWidget*) qmdw;
			shared_ptr<MixDevice> md = mdw->mixDevice();

			//qCDebug(KMIX_LOG) << "  grp=" << grp.toLatin1();
			//qCDebug(KMIX_LOG) << "  mixer=" << view->id().toLatin1();
			//qCDebug(KMIX_LOG) << "  mdwPK=" << mdw->mixDevice()->id().toLatin1();

			QString devgrp = QString("%1.%2.%3").arg(grp).arg(md->mixer()->id()).arg(md->id());
			KConfigGroup devcg = config->group(devgrp);

			if (mdw->inherits("MDWSlider"))
			{
				// only sliders have the ability to split apart in mutliple channels
				devcg.writeEntry("Split", !mdw->isStereoLinked());
			}
			/*
			if (!dynamic)
			{
				devcg.writeEntry("Show", mdw->isVisibleTo(view));
//             qCDebug(KMIX_LOG) << "Save devgrp" << devgrp << "show=" << mdw->isVisibleTo(view);
			}
			*/

		} // inherits MixDeviceWidget
	} // for all MDW's

	if (!dynamic)
	{
		// We do not save GUIProfiles (as they cannot be customized) for dynamic mixers (e.g. PulseAudio)
		if (guiProfile()->isDirty())
		{
			qCDebug(KMIX_LOG)
			<< "Writing dirty profile. grp=" << grp;
			guiProfile()->writeProfile();
		}
	}
}


// ---------- Popup stuff END ---------------------

