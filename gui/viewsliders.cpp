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

//#define TEST_MIXDEVICE_COMPOSITE
#undef TEST_MIXDEVICE_COMPOSITE

#ifdef TEST_MIXDEVICE_COMPOSITE
#ifdef __GNUC__
#warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#warning !!! MIXDEVICE COMPOSITE TESTING IS ACTIVATED   !!!
#warning !!! THIS IS PRE-ALPHA CODE!                    !!!
#warning !!! DO NOT SHIP KMIX IN THIS STATE             !!!
#warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif
#endif

#include "viewsliders.h"

// KMix
#include "core/ControlManager.h"
#include "core/mixdevicecomposite.h"
#include "core/mixer.h"
#include "gui/guiprofile.h"
#include "gui/mdwenum.h"
#include "settings.h"
#include "gui/mdwslider.h"

// KDE
#include <klocalizedstring.h>
#include <kmessagewidget.h>

// Qt
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

/**
 * Generic View implementation. This can hold now all kinds of controls (not just Sliders, as
 *  the class name suggests).
 */
ViewSliders::ViewSliders(QWidget *parent, const QString &id, Mixer *mixer,
			 ViewBase::ViewFlags vflags, const QString &guiProfileId,
			 KActionCollection *actColl)
    : ViewBase(parent, id, Qt::FramelessWindowHint, vflags, guiProfileId, actColl)
{
	addMixer(mixer);

	m_configureViewButton = nullptr;
	m_layoutMDW = nullptr;
	m_layoutSliders = nullptr;
	m_layoutSwitches = nullptr;
	m_emptyStreamHint = nullptr;

	createDeviceWidgets();

	ControlManager::instance().addListener(mixer->id(),
					       ControlManager::GUI|ControlManager::ControlList|ControlManager::Volume,
					       this, QString("ViewSliders.%1").arg(mixer->id()));
}

ViewSliders::~ViewSliders()
{
    ControlManager::instance().removeListener(this);
    delete m_layoutMDW;
}

void ViewSliders::controlsChange(ControlManager::ChangeType changeType)
{
	switch (changeType)
	{
	case ControlManager::ControlList:
		createDeviceWidgets();
		break;
	case ControlManager::GUI:
		updateGuiOptions();
		break;

	case ControlManager::Volume:
		if (Settings::debugVolume())
			qCDebug(KMIX_LOG)
			<< "NOW I WILL REFRESH VOLUME LEVELS. I AM " << id();
		refreshVolumeLevels();
		break;

	default:
		ControlManager::warnUnexpectedChangeType(changeType, this);
		break;
	}

}


QWidget *ViewSliders::add(const shared_ptr<MixDevice> md)
{
    MixDeviceWidget *mdw;

    if (md->isEnum())					// control is a switch
    {
        mdw = new MDWEnum(md, {}, this);
        m_layoutSwitches->addWidget(mdw);
    }
    else						// control is a slider
    {
        mdw = new MDWSlider(md, MixDeviceWidget::ShowMute|MixDeviceWidget::ShowCapture, this);
        m_layoutSliders->addWidget(mdw);
    }

    return (mdw);
}


void ViewSliders::initLayout()
{
	resetMdws();

	// Our m_layoutSliders now should only contain spacer widgets from the addSpacing() calls in add() above.
	// We need to trash those too otherwise all sliders gradually migrate away from the edge :p
	if (m_layoutSliders!=nullptr)
	{
		QLayoutItem *li;
		while ((li = m_layoutSliders->takeAt(0))!=nullptr) delete li;
		m_layoutSliders = nullptr;
	}


	delete m_configureViewButton;
	m_configureViewButton = nullptr;

	if (m_layoutSwitches!=nullptr)
	{
		QLayoutItem *li;
		while ((li = m_layoutSwitches->takeAt(0))!=nullptr) delete li;
		m_layoutSwitches = nullptr;
	}

	delete m_layoutMDW;
	m_layoutMDW = new QGridLayout(this);
	m_layoutMDW->setContentsMargins(0, 0, 0, 0);
	m_layoutMDW->setSpacing(0);
	m_layoutMDW->setRowStretch(0, 1);
	m_layoutMDW->setColumnStretch(0, 1);

	if (orientation()==Qt::Horizontal)		// horizontal sliders
	{
		m_layoutMDW->setAlignment(Qt::AlignLeft|Qt::AlignTop);

		m_layoutSliders = new QVBoxLayout();
		m_layoutSliders->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
	}
	else						// vertical sliders
	{
		m_layoutMDW->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

		m_layoutSliders = new QHBoxLayout();
		m_layoutSliders->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	}
	m_layoutSliders->setContentsMargins(0, 0, 0, 0);
	m_layoutSliders->setSpacing(0);

	m_layoutSwitches = new QVBoxLayout();

	QString emptyStreamText = i18n("Nothing is playing audio.");
	// Hint: This text comparison is not a clean solution, but one that will work for quite a while.
	const QString viewId = id();
	if (viewId.contains(".Capture_Streams.")) emptyStreamText = i18n("Nothing is capturing audio.");
	else if (viewId.contains(".Capture_Devices.")) emptyStreamText = i18n("There are no capture devices.");
	else if (viewId.contains(".Playback_Devices.")) emptyStreamText = i18n("There are no playback devices.");

	delete m_emptyStreamHint;
	m_emptyStreamHint = new KMessageWidget(emptyStreamText, this);
	m_emptyStreamHint->setIcon(QIcon::fromTheme("dialog-information"));
	m_emptyStreamHint->setMessageType(KMessageWidget::Information);
	m_emptyStreamHint->setCloseButtonVisible(false);
	m_emptyStreamHint->setMaximumWidth(200);
	m_emptyStreamHint->setWordWrap(true);
	m_layoutSliders->addWidget(m_emptyStreamHint);

	if (orientation()==Qt::Horizontal)		// horizontal sliders
	{
		// Row 0: Sliders
		m_layoutMDW->addLayout(m_layoutSliders, 0, 0, 1, -1, Qt::AlignHCenter|Qt::AlignVCenter);
		// Row 1: Switches
		m_layoutMDW->addLayout(m_layoutSwitches, 1, 0, Qt::AlignLeft|Qt::AlignTop);
	}
	else						// vertical sliders
	{
		// Column 0: Sliders
		m_layoutMDW->addLayout(m_layoutSliders, 0, 0, -1, 1, Qt::AlignHCenter|Qt::AlignVCenter);
		// Column 1: Switches
		m_layoutMDW->addLayout(m_layoutSwitches, 0, 1, Qt::AlignLeft|Qt::AlignTop);
	}

#ifdef TEST_MIXDEVICE_COMPOSITE
	QList<shared_ptr<MixDevice> > mds;		// For temporary test
#endif

	// This method iterates over all the controls from the GUI profile.
	// Each control is checked, whether it is also contained in the mixset, and
	// applicable for this kind of View. If yes, the control is accepted and inserted.

	const GUIProfile *guiprof = guiProfile();
	if (guiprof!=nullptr)
	{
		for (const Mixer *mixer : getMixers())
		{
			const MixSet &mixset = mixer->getMixSet();

			for (ProfControl *control : qAsConst(guiprof->getControls()))
			{
				// The TabName of the control matches this View name (!! attention: Better use some ID, due to i18n() )
				QRegExp idRegexp(control->id());
				// The following for-loop could be simplified by using a std::find_if
				for (int i = 0; i<mixset.count(); ++i)
				{
					const shared_ptr<MixDevice> md = mixset[i];

					if (md->id().contains(idRegexp))
					{		// match found (by name)
						if (getMixSet().contains(md))
						{	// check for duplicate already
							continue;
						}

						// Now check whether subcontrols required
						const bool subcontrolPlaybackWanted = (control->useSubcontrolPlayback() && (md->playbackVolume().hasVolume() || md->hasMuteSwitch()));
						const bool subcontrolCaptureWanted = (control->useSubcontrolCapture() && (md->captureVolume().hasVolume() || md->captureVolume().hasSwitch()));
						const bool subcontrolEnumWanted = (control->useSubcontrolEnum() && md->isEnum());
						const bool subcontrolWanted = subcontrolPlaybackWanted || subcontrolCaptureWanted || subcontrolEnumWanted;
						if (!subcontrolWanted) continue;

						md->setControlProfile(control);
						if (!control->name().isNull())
						{
							// Apply the custom name from the profile
							// TODO:  This is the wrong place.  It only
							// applies to controls in THIS type of view.
							md->setReadableName(control->name());
						}
						if (!control->getSwitchtype().isNull())
						{
							if (control->getSwitchtype()=="On")
								md->playbackVolume().setSwitchType(Volume::OnSwitch);
							else if (control->getSwitchtype()=="Off")
								md->playbackVolume().setSwitchType(Volume::OffSwitch);
						}
						addToMixSet(md);

#ifdef TEST_MIXDEVICE_COMPOSITE
						if ( md->id() == "Front:0" || md->id() == "Surround:0")
						{	// For temporary test
							mds.append(md);
						}
#endif
						// No 'break' here, because multiple devices could match
						// the regexp (e.g. "^.*$")
					}		// name matches
				}			// loop for finding a suitable MixDevice
			}				// iteration over all controls from the Profile
		}					// iteration over all Mixers
	}						// if there is a profile

	// Show a hint why a tab is empty (dynamic controls!)
	// TODO: 'visibleControls()==0' could be used for the !isDynamic() case
	m_emptyStreamHint->setVisible(getMixSet().isEmpty() && isDynamic());

#ifdef TEST_MIXDEVICE_COMPOSITE
	// @todo: This is currently hardcoded, and instead must be read as usual from the Profile
	MixDeviceComposite *mdc = new MixDeviceComposite(_mixer, "Composite_Test", mds, "A Composite Control #1", MixDevice::KMIX_COMPOSITE);
	Volume::ChannelMask chn = Volume::MMAIN;
	Volume* vol = new Volume( chn, 0, 100, true, true);
	mdc->addPlaybackVolume(*vol);
	QString ctlId("Composite_Test");
	QString ctlMatchAll("*");
	ProfControl* pctl = new ProfControl(ctlId, ctlMatchAll);
	mdc->setControlProfile(pctl);
	_mixSet->append(mdc);
#endif
}


void ViewSliders::constructionFinished()
{
	m_layoutSwitches->addStretch(1);		// push switches to top or left
	configurationUpdate();
	if (!isDynamic())
	{
		// Layout row 1 column 1: Configure View button
		// TODO: does this need to be a member?
		m_configureViewButton = createConfigureViewButton();
		m_layoutMDW->addWidget(m_configureViewButton, 1, 1, Qt::AlignRight|Qt::AlignBottom);
	}

	updateGuiOptions();
}


void ViewSliders::configurationUpdate()
{
	const int num = mixDeviceCount();

	// Set the visibility of all controls
	for (int i = 0; i<num; ++i)
	{
		MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(mixDeviceAt(i));
		if (mdw==nullptr) continue;

		// The GUI level has been set earlier, by inspecting the controls
		const ProfControl *matchingControl = findMdw(mdw->mixDevice()->id());
		mdw->setVisible(matchingControl!=nullptr);
	}
	// Then adjust the controls layout
	adjustControlsLayout();
}


void ViewSliders::refreshVolumeLevels()
{
	const int num = mixDeviceCount();
	for (int i = 0; i<num; ++i)
	{
		MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget*>(mixDeviceAt(i));
		if (mdw!=nullptr)
		{ // sanity check

#ifdef TEST_MIXDEVICE_COMPOSITE
			// --- start --- The following 4 code lines should be moved to a more
			//                      generic place, as it only works in this View. But it
			//                      should also work in the ViewDockareaPopup and everywhere else.
			MixDeviceComposite* mdc = ::qobject_cast<MixDeviceComposite*>(mdw->mixDevice());
			if (mdc != 0)
			{
				mdc->update();
			}
			// --- end ---
#endif

			if (Settings::debugVolume())
			{
				bool debugMe = (mdw->mixDevice()->id() == "PCM:0");
				if (debugMe)
					qCDebug(KMIX_LOG)
					<< "Old PCM:0 playback state" << mdw->mixDevice()->isMuted() << ", vol="
						<< mdw->mixDevice()->playbackVolume().getAvgVolumePercent(Volume::MALL);
			}

			mdw->update();
		}
		else
		{
			qCCritical(KMIX_LOG) << "ViewSliders::refreshVolumeLevels(): mdw is not a MixDeviceWidget\n";
			// no slider. Cannot happen in theory => skip it
		}
	}
}


Qt::Orientation ViewSliders::orientationSetting() const
{
	return (static_cast<Qt::Orientation>(Settings::orientationMainWindow()));
}
