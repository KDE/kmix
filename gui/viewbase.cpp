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

// Qt
#include <qcursor.h>
#include <QMenu>
#include <QMouseEvent>

// KDE
#include <klocalizedstring.h>
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
#include "gui/mdwenum.h"
#include "core/ControlManager.h"
#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "settings.h"


/**
 * Creates an empty View. To populate it with MixDevice instances, you must implement
 * initLayout() in your derived class.
 */
ViewBase::ViewBase(QWidget* parent, const QString &id, Qt::WindowFlags f, ViewBase::ViewFlags vflags, const QString &guiProfileId, KActionCollection *actionColletion)
    : QWidget(parent, f),
      _popMenu(NULL),
      _actions(actionColletion),
      _vflags(vflags),
      guiLevel(GuiVisibility::Simple),
      _guiProfileId(guiProfileId)
{
   setObjectName(id);
   qCDebug(KMIX_LOG) << "id" << id << "flags" << vflags;

   // When loading the View from the XML profile, guiLevel can get overridden
   m_viewId = id;
   
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


void ViewBase::addMixer(Mixer *mixer)
{
  _mixers.append(mixer);
}


QPushButton* ViewBase::createConfigureViewButton()
{
	QPushButton* configureViewButton = new QPushButton(QIcon::fromTheme(QLatin1String("configure")),
							   QString(), this);
	configureViewButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	configureViewButton->setToolTip(i18n( "Configure this view" ));
	connect(configureViewButton, SIGNAL(clicked(bool)), SLOT(configureView()));
	return configureViewButton;
}

void ViewBase::updateGuiOptions()
{
    setTicks(Settings::showTicks());
    setLabels(Settings::showLabels());
    updateMediaPlaybackIcons();
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
    // If this widget is currently visible, it is hidden while the device widgets are
    // being added and shown again afterwards.  This is because, if we are called as
    // a result of change of orientation or a control being added or removed while we
    // are visible, isVisibleTo() in ViewSliders::configurationUpdate() appears to
    // return false even for controls that should be visible.  This means that they
    // do not get included in the label extent calculation and the labels will not
    // be resized evenly.
    const bool wasVisible = isVisible();
    if (wasVisible) hide();				// hide if currently visible

    _orientation = orientationSetting();		// refresh orientation from settings
    qCDebug(KMIX_LOG) << id() << "orientation" << _orientation;

    initLayout();
    for (const shared_ptr<MixDevice> md : qAsConst(_mixSet))
    {
        QWidget *mdw = add(md);				// a) Let the implementation do its work
        _mdws.append(mdw);				// b) Add it to the local list
        connect(mdw, SIGNAL(guiVisibilityChange(MixDeviceWidget*,bool)), SLOT(guiVisibilitySlot(MixDeviceWidget*,bool)));
    }

    if (!isDynamic())
    {
        QAction *action = _localActionColletion->addAction("toggle_channels");
        action->setText(i18n("Configure Channels..."));
        connect(action, SIGNAL(triggered(bool)), SLOT(configureView()));
    }

    constructionFinished();				// allow view to "polish" itself
    if (wasVisible) show();				// show again if originally visible
}


void ViewBase::adjustControlsLayout()
{
	// Adjust the view layout by setting the extent of all the control labels
	// to allow space for the largest.  The extent of a control's label is
	// found from its labelExtentHint() virtual function (which takes account
	// of the control layout direction), and is then set by its setLabelExtent()
	// virtual function.

	// The maximum extent is calculated and set separately for sliders and
	// for switches (enums).
	int labelExtentSliders = 0;
	int labelExtentSwitches = 0;

	const int num = mixDeviceCount();

	// Pass 1: Find the maximum extent of all applicable controls
	for (int i = 0; i<num; ++i)
	{
		const QWidget *w = mixDeviceAt(i);
		Q_ASSERT(w!=nullptr);
		if (!w->isVisibleTo(this)) continue;	// not currently visible

		const MDWSlider *mdws = qobject_cast<const MDWSlider *>(w);
		if (mdws!=nullptr) labelExtentSliders = qMax(labelExtentSliders, mdws->labelExtentHint());

		const MDWEnum *mdwe = qobject_cast<const MDWEnum *>(w);
		if (mdwe!=nullptr) labelExtentSwitches = qMax(labelExtentSwitches, mdwe->labelExtentHint());
	}

	// Pass 2: Set the maximum extent of all applicable controls
	for (int i = 0; i<num; ++i)
	{
		QWidget *w = mixDeviceAt(i);
		Q_ASSERT(w!=nullptr);
		if (!w->isVisibleTo(this)) continue;	// not currently visible

		MDWSlider *mdws = qobject_cast<MDWSlider *>(w);
		if (mdws!=nullptr && labelExtentSliders>0) mdws->setLabelExtent(labelExtentSliders);

		MDWEnum *mdwe = qobject_cast<MDWEnum *>(w);
		if (mdwe!=nullptr && labelExtentSwitches>0) mdwe->setLabelExtent(labelExtentSwitches);
	}

	// An old comment in ViewSliders::configurationUpdate() said
	// that this was necessary for KDE3.  Not sure if it is still
	// required two generations later.
	layout()->activate();
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
	ProfControl *pctl = findMdw(md->id());
	if (pctl==nullptr)
	{
		qCWarning(KMIX_LOG) << "MixDevice not found, and cannot be hidden, id=" << md->id();
		return; // Ignore
	}

	pctl->setVisible(enable);
	ControlManager::instance().announce(md->mixer()->id(), ControlManager::ControlList, QString("ViewBase::guiVisibilitySlot"));
}

// // ---------- Popup stuff START ---------------------

void ViewBase::contextMenuEvent(QContextMenuEvent *ev)
{
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

// ---------- Popup stuff END ---------------------


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
	for (const Mixer *mixer : qAsConst(_mixers))
	{
		if (mixer->isDynamic()) return true;
	}
	return false;
}

bool ViewBase::pulseaudioPresent() const
{
	// We do not use Mixer::pulseaudioPresent(), as we are only interested in Mixer instances contained in this View.
	for (const Mixer *mixer : qAsConst(_mixers))
	{
		if ( mixer->getDriverName() == "PulseAudio" ) return true;
	}
	return false;
}


void ViewBase::resetMdws()
{
      // We need to delete the current MixDeviceWidgets so we can recreate them
//       while (!_mdws.isEmpty())
// 	      delete _mdws.takeFirst();
      qDeleteAll(_mdws);
      _mdws.clear();

      // _mixSet contains shared_ptr instances, so clear() should be enough to prevent mem leak
      _mixSet.clear(); // Clean up our _mixSet so we can reapply our GUIProfile
}


int ViewBase::visibleControls() const
{
	int visibleCount = 0;
	for (const QWidget *qw : qAsConst(_mdws))
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
void ViewBase::load(const KConfig *config)
{
	ViewBase *view = this;
	QString grp = "View.";
	grp += view->id();
	//KConfigGroup cg = config->group( grp );
	qCDebug(KMIX_LOG)
	<< "KMixToolBox::loadView() grp=" << grp.toLatin1();

	static const GuiVisibility guiVisibilities[3] =
	{
		GuiVisibility::Simple,
		GuiVisibility::Extended,
		GuiVisibility::Full
	};

	bool guiLevelSet = false;
	for (int i = 0; i<3; ++i)
	{
		const GuiVisibility guiCompl = guiVisibilities[i];
		bool atLeastOneControlIsShown = false;
		for (QWidget *qmdw : qAsConst(view->_mdws))
		{
			MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(qmdw);
			if (mdw!=nullptr)
			{
				shared_ptr<MixDevice> md = mdw->mixDevice();
				QString devgrp = md->configGroupName(grp);
				KConfigGroup devcg = config->group(devgrp);

				if (mdw->inherits("MDWSlider"))
				{
					// only sliders have the ability to split apart in multiple channels
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
		setGuiLevel(GuiVisibility::Full);
}

void ViewBase::setGuiLevel(GuiVisibility guiLevel)
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
ProfControl *ViewBase::findMdw(const QString& mdwId, GuiVisibility visibility) const
{
	for (ProfControl *pControl : qAsConst(guiProfile()->getControls()))
	{
		QRegExp idRegExp(pControl->id());
		if ( mdwId.contains(idRegExp) )
		{
			if (pControl->satisfiesVisibility(visibility))
			{
//				qCDebug(KMIX_LOG) << "  MATCH " << (*pControl).id << " for " << mdwId << " with visibility " << pControl->getVisibility().getId() << " to " << visibility.getId();
				return pControl;
			}
			else
			{
//				qCDebug(KMIX_LOG) << "NOMATCH " << (*pControl).id << " for " << mdwId << " with visibility " << pControl->getVisibility().getId() << " to " << visibility.getId();
			}
		}
	}						// iterate over all ProfControl entries
	return (nullptr);				// not found
}


/*
 * Saves the View configuration
 */
void ViewBase::save(KConfig *config) const
{
	const QString grp = "View."+id();

	// Certain bits are not saved for dynamic mixers (e.g. PulseAudio)
	bool dynamic = isDynamic();  // TODO 11 Dynamic view configuration

	for (int i = 0; i<_mdws.count(); ++i)
	{
                MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(_mdws[i]);
		if (mdw!=nullptr)
		{
			shared_ptr<MixDevice> md = mdw->mixDevice();

			//qCDebug(KMIX_LOG) << "  grp=" << grp.toLatin1();
			//qCDebug(KMIX_LOG) << "  mixer=" << view->id().toLatin1();
			//qCDebug(KMIX_LOG) << "  mdwPK=" << mdw->mixDevice()->id().toLatin1();

			QString devgrp = QString("%1.%2.%3").arg(grp, md->mixer()->id(), md->id());
			KConfigGroup devcg = config->group(devgrp);

			if (mdw->inherits("MDWSlider"))
			{
				// only sliders have the ability to split apart in multiple channels
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
