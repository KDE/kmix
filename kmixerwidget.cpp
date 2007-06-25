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
#include <QLayout>
#include <qpixmap.h>
#include <qslider.h>
#include <QString>
#include <qtoolbutton.h>
#include <QToolTip>
#include <qapplication.h> // for QApplication::revsreseLayout()

// KDE
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>
#include <ktabwidget.h>

// KMix
#include "guiprofile.h"
#include "kmixerwidget.h"
#include "kmixtoolbox.h"
#include "mixdevicewidget.h"
#include "mixer.h"
#include "mixertoolbox.h"
#include "viewsliders.h"
// KMix experimental
//#include "viewsurround.h"


/**
   This widget is embedded in the KMix Main window. Each Hardware Card is visualized by one KMixerWidget.
   KMixerWidget contains
   (a)  A TabBar with n Tabs (at least one per soundcard). These contain View's with sliders, switches and other GUI elements visualizing the Mixer)
   (b) A balancing slider : This will be moved to ViewSliders.
*/
KMixerWidget::KMixerWidget( Mixer *mixer,
                            QWidget * parent, const char * name, ViewBase::ViewFlags vflags, KActionCollection* actionCollection )
   : QWidget( parent ), _mixer(mixer), m_balanceSlider(0),
     m_topLayout(0), _actionCollection(actionCollection), 
     _iconsEnabled( true ), _labelsEnabled( false ), _ticksEnabled( false )

{
   setObjectName(name);

   if ( _mixer )
   {
      m_id = _mixer->id();
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
   if( m_balanceSlider ) {
      delete m_balanceSlider;
   }
   if( m_topLayout ) {
      delete m_topLayout;
   }

   // create main layout
   m_topLayout = new QVBoxLayout( this );
   m_topLayout->setSpacing( 3 );
   m_topLayout->setObjectName( "m_topLayout" );

   /*******************************************************************
   *  Now the main GUI is created.
   * 1) Select a (GUI) profile,  which defines  which controls to show on which Tab
   * 2a) Create the Tab's and the corresponding Views
   * 2b) Create device widgets
   * 2c) Add Views to Tab
   ********************************************************************/
   GUIProfile* guiprof = MixerToolBox::instance()->selectProfile(_mixer);
   createViewsByProfile(_mixer, guiprof, vflags);


   // *** Lower part: Slider and Mixer Name ************************************************
   QHBoxLayout *balanceAndDetail = new QHBoxLayout();
   m_topLayout->addItem( balanceAndDetail );
   balanceAndDetail->setObjectName( "balanceAndDetail" );
   balanceAndDetail->setSpacing( 8 );
   // Create the left-right-slider
   m_balanceSlider = new QSlider( Qt::Horizontal, this );
   m_balanceSlider->setMinimum(-100);
   m_balanceSlider->setMaximum(100);
   m_balanceSlider->setPageStep(25);
   m_balanceSlider->setValue(0);

   m_balanceSlider->setObjectName("RightLeft"); 
   m_balanceSlider->setTickPosition( QSlider::TicksBelow );
   m_balanceSlider->setTickInterval( 25 );
   m_balanceSlider->setMinimumSize( m_balanceSlider->sizeHint() );
   m_balanceSlider->setFixedHeight( m_balanceSlider->sizeHint().height() );

/*
   QLabel *mixerName = new QLabel(this );
   mixerName->setObjectName("mixerName");
   mixerName->setText( _mixer->readableName() );
   mixerName->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
*/

   // 10 Pixels at the front; Balance-Slider; Mixer-Name; 10 Pixels at the end
   balanceAndDetail->addSpacing( 10 );
   balanceAndDetail->addWidget( m_balanceSlider );
//   balanceAndDetail->addWidget( mixerName );
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

   show();
   //    kDebug(67100) << "KMixerWidget::createLayout(): EXIT\n";
}


const QString& KMixerWidget::id() const {
   return m_id;
}

/**
 * Creates all the Views for the Tabs described in the GUIProfile
 */
void KMixerWidget::createViewsByProfile(Mixer* mixer, GUIProfile *guiprof, ViewBase::ViewFlags vflags)
{
   /*** How it works:
   * A loop is done over all tabs.
   * For each Tab a View (e.g. ViewSliders) is instanciated and added to the list of Views
   */
   std::vector<ProfTab*>::const_iterator itEnd = guiprof->_tabs.end();
   for ( std::vector<ProfTab*>::const_iterator it = guiprof->_tabs.begin(); it != itEnd; ++it) {
      ProfTab* profTab = *it;

      // The i18n() in the next line will only produce a translated version, if the text is known.
      // This cannot be guaranteed, as we have no *.po-file, and the value is taken from the XML Profile.
      // It is possible that the Profile author puts arbitrary names in it.
      kDebug(67100) << "KMixerWidget::createViewsByProfile() add " << profTab->type.toUtf8() << " name="<<profTab->name.toUtf8() << "\n";
      if ( profTab->type == "Sliders" ) {
         ViewSliders* view = new ViewSliders( this, profTab->name.toAscii(), mixer, vflags, guiprof, _actionCollection );
         possiblyAddView(view, profTab->name);
      }
// Disable this View until the rest is done for KDE4
//      else if ( profTab->type == "Surround" ) {
//         ViewSurround* view = new ViewSurround ( this, profTab->name.toAscii(), mixer, vflags, guiprof, _actionCollection );
//         possiblyAddView(view, profTab->name);
//      }

      else {
         kDebug(67100) << "KMixerWidget::createViewsByProfile(): Unknown Tab type '" << profTab->type << "'\n";
      }
   } // for all tabs
}

void KMixerWidget::possiblyAddView(ViewBase* vbase, QString& tabName)
{
   if ( ! vbase->isValid()  )
      delete vbase;
   else {
      vbase->createDeviceWidgets();
      //vbase->show();
      m_topLayout->addWidget(vbase);

/*
      QString finalTabName;
      finalTabName = i18n(tabName.toAscii());  // This is a dynamic i18n(). Usual texts are "Play", "Record", and "Switches"
      finalTabName += " (" + _mixer->readableName() + ")";
      //m_ioTab->addTab( vbase , finalTabName ); //  @todo move to class KMix
*/
      connect( vbase, SIGNAL(toggleMenuBar()), parentWidget(), SLOT(toggleMenuBar()) );
   }
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
    if ( _labelsEnabled!=on ) {
		// value was changed
		_labelsEnabled = on;
		const std::vector<ViewBase*>::const_iterator viewsEnd = _views.end();
		for ( std::vector<ViewBase*>::const_iterator it = _views.begin(); it != viewsEnd; ++it) {
			ViewBase* mixerWidget = *it;
			mixerWidget->setLabels(on);
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
		    mixerWidget->setTicks(on);
		} // for all tabs
    }
}


/**
 *  @todo : Is the view list already filled, when loadConfig() is called?
 */
void KMixerWidget::loadConfig( KConfig *config, const QString &grp )
{
        config->setGroup( grp );

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
	config->writeEntry("Mixer_Name_Key", _mixer->id());

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
// hack around it before calling _mixer->setBalance()
void KMixerWidget::balanceChanged(int balance)
{
    if (QApplication::isRightToLeft())
        balance = -balance;

    _mixer->setBalance( balance );
}

#include "kmixerwidget.moc"
