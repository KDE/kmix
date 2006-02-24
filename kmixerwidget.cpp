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
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qslider.h>
#include <qstring.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qapplication.h> // for QApplication::revsreseLayout()

// KDE
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ktabwidget.h>

// KMix
#include "guiprofile.h"
#include "kmixerwidget.h"
#include "kmixtoolbox.h"
#include "mixdevicewidget.h"
#include "mixer.h"
#include "mixertoolbox.h"
#include "viewinput.h"
#include "viewoutput.h"
#include "viewsliderset.h"
#include "viewswitches.h"
// KMix experimental
//#include "viewgrid.h"
#include "viewsurround.h"


/**
   This widget is embedded in the KMix Main window. Each Hardware Card is visualized by one KMixerWidget.
   KMixerWidget contains
   (a) a headline where you can change Mixer's (if you got more than one Mixer)
   (b) a Tab with 2-4 Tabs (containing View's with sliders, switches and other GUI elements visualizing the Mixer)
   (c) A balancing slider
   (d) A label containg the mixer name
*/
KMixerWidget::KMixerWidget( Mixer *mixer,
                            MixDevice::DeviceCategory categoryMask,
                            QWidget * parent, const char * name, ViewBase::ViewFlags vflags )
   : QWidget( parent, name ), _mixer(mixer), m_balanceSlider(0),
     m_topLayout(0),
     _iconsEnabled( true ), _labelsEnabled( false ), _ticksEnabled( false )

{
    m_categoryMask = categoryMask;
    m_id = mixer->mixerName();  // !!! A BETTER ID must probably be found here

   if ( _mixer )
   {
      createLayout(vflags);
   }
   else
   {
       // No mixer found
       // !! Fix this: This is actually never shown!
       QBoxLayout *layout = new QHBoxLayout( this );
       QString s = i18n("Invalid mixer");
       if ( !mixer->mixerName().isEmpty() )
	   s.append(" \"").append(mixer->mixerName()).append("\"");
       QLabel *errorLabel = new QLabel( s, this );
       errorLabel->setAlignment( Qt::AlignCenter | Qt::TextWordWrap );
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
    if( m_balanceSlider ) {
	delete m_balanceSlider;
    }
    if( m_topLayout ) {
	delete m_topLayout;
    }

    // create main layout
    m_topLayout = new QVBoxLayout( this, 0, 3,  "m_topLayout" );

    // Create tabs of input + output + [...]
    m_ioTab = new KTabWidget( this);
	m_ioTab->setObjectName( "ioTab" );


    QToolButton* m_profileButton = new QToolButton( m_ioTab );
    QToolTip::add(m_profileButton,i18n("Click for selecting the next profile.\nClick and hold for profile menu."));
    m_profileButton->setIconSet( SmallIcon( "tab_new" ) );
    m_profileButton->adjustSize();
    // !!! m_profileButton->setPopup( m_tabbarSessionsCommands );
    connect(m_profileButton, SIGNAL(clicked()), SLOT(newSession()));
    m_ioTab->setCornerWidget( m_profileButton, Qt::BottomLeftCorner );

    QToolButton* m_closeButton = new QToolButton( m_ioTab );
    QToolTip::add(m_closeButton,i18n("Close Tab"));
    m_closeButton->setIconSet( SmallIcon( "tab_remove" ) );
    m_closeButton->adjustSize();
    connect(m_closeButton, SIGNAL(clicked()), SLOT(removeSession()));
    m_ioTab->setCornerWidget( m_closeButton, Qt::TopRightCorner );


    m_profileButton->installEventFilter(this);
    m_topLayout->add( m_ioTab );


     /*******************************************************************
      *  Now the main GUI is created.
      * 1) Select a (GUI) profile,  which defines  which controls to show on which Tab
      * 2a) Create the Tab's and the corresponding Views
      * 2b) Create device widgets
      * 2c) Add Views to Tab
      ********************************************************************/
      GUIProfile* guiprof = MixerToolBox::selectProfile(_mixer);
      if ( guiprof != 0 ) {
	createViewsByProfile(_mixer, guiprof, vflags);
      }
      else
      {
	// Fallback, if no GUI Profile could be found
	possiblyAddView(new ViewOutput  ( m_ioTab, "Output" , _mixer, vflags, 0 ) );
	possiblyAddView(new ViewInput( m_ioTab, "Input"  , _mixer, vflags, 0 ) );
	possiblyAddView(new ViewSwitches( m_ioTab, "Switches" , _mixer, vflags, 0 ) );
	if ( vflags & ViewBase::Experimental_SurroundView )
		possiblyAddView( new ViewSurround( m_ioTab, "Surround", _mixer, vflags, 0 ) );
	//!!if ( vflags & ViewBase::Experimental_GridView )
	//!!	possiblyAddView( new ViewGrid( m_ioTab, "Grid", _mixer, vflags, 0 ) );
      }



    // *** Lower part: Slider and Mixer Name ************************************************
    QHBoxLayout *balanceAndDetail = new QHBoxLayout( m_topLayout, 8,  "balanceAndDetail");
    // Create the left-right-slider
    m_balanceSlider = new QSlider( -100, 100, 25, 0, Qt::Horizontal, this, "RightLeft" );
    m_balanceSlider->setTickmarks( QSlider::TicksBelow );
    m_balanceSlider->setTickInterval( 25 );
    m_balanceSlider->setMinimumSize( m_balanceSlider->sizeHint() );
    m_balanceSlider->setFixedHeight( m_balanceSlider->sizeHint().height() );

    QLabel *mixerName = new QLabel(this, "mixerName");
    mixerName->setText( _mixer->mixerName() );
    mixerName->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    balanceAndDetail->addSpacing( 10 );

    balanceAndDetail->addWidget( m_balanceSlider );
    balanceAndDetail->addWidget( mixerName );
    balanceAndDetail->addSpacing( 10 );

    connect( m_balanceSlider, SIGNAL(valueChanged(int)), this, SLOT(balanceChanged(int)) );
    m_balanceSlider->setToolTip( i18n("Left/Right balancing") );

    /* @todo : update all Background Pixmaps
    const QPixmap bgPixmap = UserIcon("bg_speaker");
    setBackgroundPixmap ( bgPixmap );
    const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
    for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
	    ViewBase* view = *it;
	    view->setBackgroundPixmap ( bgPixmap );
    } // for all Views
    */ 
    
    // --- "MenuBar" toggling from the various View's ---



    show();
    //    kDebug(67100) << "KMixerWidget::createLayout(): EXIT\n";
}


const QString& KMixerWidget::id() const {
	return m_id;
}


/**
 * Creates all the Views for the Tabs described in the GUIProfile
 */
void KMixerWidget::createViewsByProfile(Mixer* mixer, GUIProfile *guiprof, ViewBase::ViewFlags vflags) // !!! Impl. pending
{
	/*** How it works:
	 * A loop is done over all tabs.
	 * For each Tab a View (e.g. ViewSliderSet) is instanciated and added to the list of Views
	 */
	std::vector<ProfTab*>::const_iterator itEnd = guiprof->_tabs.end();
	for ( std::vector<ProfTab*>::const_iterator it = guiprof->_tabs.begin(); it != itEnd; ++it) {
		ProfTab* profTab = *it;

		// The i18n() in the next line will only produce a translated version, if the text is known.
		// This cannot be guaranteed, as we have no *.po-file, and the value is taken from the XML Profile.
		// It is possible that the Profile author puts arbitrary names in it.
		kDebug(67100) << "KMixerWidget::createViewsByProfile() add " << profTab->type.utf8() << "name="<<profTab->name.utf8() << "\n";
		if ( profTab->type == "SliderSet" ) {
			ViewSliderSet* view = new ViewSliderSet  ( m_ioTab, profTab->name.utf8(), mixer, vflags, guiprof );
			possiblyAddView(view);
		}
		else if ( profTab->type == "Surround" ) {
			ViewSurround* view = new ViewSurround (m_ioTab, profTab->name.utf8(), mixer, vflags, guiprof );
			possiblyAddView(view);
		}
		/*
		else if ( profTab->type == "Switches" ) {
			ViewSliderSet* view = new ViewSwitchSet  ( m_ioTab, profTab->name, _mixer, vflags, guiprof );
			possiblyAddView(view);
		}
		*/
		else {
			kDebug(67100) << "KMixerWidget::createViewsByProfile(): Unknown Tab type '" << profTab->type << "'\n";
		}
	} // for all tabs
}

void KMixerWidget::possiblyAddView(ViewBase* vbase)
{
	if ( vbase->count() == 0 )
		delete vbase;
	else {
		_views.push_back(vbase);
		vbase ->createDeviceWidgets();
		m_ioTab->addTab( vbase , i18n(vbase->name()) );
		connect( vbase, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
	}
}

void KMixerWidget::setIcons( bool on )
{
	const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
	for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
		ViewBase* mixerWidget = *it;
		KMixToolBox::setIcons(mixerWidget->_mdws, on);
    } // for all tabs
}

void KMixerWidget::setLabels( bool on )
{
    if ( _labelsEnabled!=on ) {
		// value was changed
		_labelsEnabled = on;
		const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
		for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
			ViewBase* mixerWidget = *it;
			KMixToolBox::setLabels(mixerWidget->_mdws, on);
	    } // for all tabs
    }
}

void KMixerWidget::setTicks( bool on )
{
    if ( _ticksEnabled!=on ) {
		// value was changed
		_ticksEnabled = on;
		const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
		for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
			ViewBase* mixerWidget = *it;
		    KMixToolBox::setTicks(mixerWidget->_mdws, on);
		} // for all tabs
    }
}


/**
 *  @todo : Is the view list already filled, when loadConfig() is called?
 */
void KMixerWidget::loadConfig( KConfig *config, const QString &grp )
{
	const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
	for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
		ViewBase* view = *it;
		KMixToolBox::loadView(view,config);
		KMixToolBox::loadKeys(view,config);
		view->configurationUpdate();
	} // for all tabs
}



void KMixerWidget::saveConfig( KConfig *config, const QString &grp )
{
	config->setGroup( grp );
	// Write mixer name. It cannot be changed in the Mixer instance,
	// it is only saved for diagnostical purposes (analyzing the config file).
	config->writeEntry("Mixer_Name_Key", _mixer->mixerName());

	const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
	for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
		ViewBase* view = *it;
		KMixToolBox::saveView(view,config);
		KMixToolBox::saveKeys(view,config);
	} // for all tabs
}


void KMixerWidget::toggleMenuBarSlot() {
    emit toggleMenuBar();
}

// in RTL mode, the slider is reversed, we cannot just connect the signal to setBalance()
// hack arround it before calling _mixer->setBalance()
void KMixerWidget::balanceChanged(int balance)
{
    if (QApplication::reverseLayout())
        balance = -balance;

    _mixer->setBalance( balance );
}

#include "kmixerwidget.moc"
