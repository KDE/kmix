/*
 * KMix -- KDE's full featured mini mixer
 *
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


// KMix
#include "mdwenum.h"
#include "core/mixer.h"
#include "viewbase.h"

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kglobalaccel.h>
#include <klocale.h>
#include <kmenu.h>
#include <ktoggleaction.h>

// Qt
#include <QCursor>
#include <QLabel>
#include <QMouseEvent>
#include <QObject>
#include <QBoxLayout>

/**
 * Class that represents an Enum element (a select one-from-many selector)
 * The orientation (horizontal, vertical) is ignored
 */
MDWEnum::MDWEnum( shared_ptr<MixDevice> md,
                 Qt::Orientation orientation,
                 QWidget* parent, ViewBase* view, ProfControl* par_pctl) :
   MixDeviceWidget(md, false, orientation, parent, view, par_pctl),
   _label(0), _enumCombo(0), _layout(0)
{
   // create actions (on _mdwActions, see MixDeviceWidget)

   // KStandardAction::showMenubar() is in MixDeviceWidget now
   KToggleAction *action = _mdwActions->add<KToggleAction>( "hide" );
   action->setText( i18n("&Hide") );
   connect(action, SIGNAL(triggered(bool)), SLOT(setDisabled()));
   QAction *c = _mdwActions->addAction( "keys" );
   c->setText( i18n("C&onfigure Shortcuts...") );
   connect(c, SIGNAL(triggered(bool)), SLOT(defineKeys()));

   // create widgets
   createWidgets();

   /* remove this for production version
     QAction *a = _mdwActions->addAction( "Next Value" );
     c->setText( i18n( "Next Value" ) );
     connect(a, SIGNAL(triggered(bool)), SLOT(nextEnumId()));
   */

   installEventFilter( this ); // filter for popup
}

MDWEnum::~MDWEnum()
{
}


void MDWEnum::createWidgets()
{
   if ( _orientation == Qt::Vertical ) {
      _layout = new QVBoxLayout( this );
	  _layout->setAlignment(Qt::AlignLeft);
   }
   else {
      _layout = new QHBoxLayout( this );
	  _layout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
   }

   _label = new QLabel( m_mixdevice->readableName(), this);
   _layout->addWidget(_label);
   _enumCombo = new KComboBox( false, this);
   _enumCombo->installEventFilter(this);
   // ------------ fill ComboBox start ------------
   int maxEnumId= m_mixdevice->enumValues().count();
   for (int i=0; i<maxEnumId; i++ ) {
      _enumCombo->addItem( m_mixdevice->enumValues().at(i));
   }
   // ------------ fill ComboBox end --------------
   _layout->addWidget(_enumCombo);
   connect( _enumCombo, SIGNAL(activated(int)), this, SLOT(setEnumId(int)) );
   _enumCombo->setToolTip( m_mixdevice->readableName() );
	_layout->addStretch(1);
}

void MDWEnum::update()
{
  if ( m_mixdevice->isEnum() ) {
    //kDebug(67100) << "MDWEnum::update() enumID=" << m_mixdevice->enumId();
    _enumCombo->setCurrentIndex( m_mixdevice->enumId() );
  }
  else {
    kError(67100) << "MDWEnum::update() enumID=" << m_mixdevice->enumId() << " is no Enum ... skipped" << endl;
  }
}

void MDWEnum::showContextMenu(const QPoint& pos )
{
   if( m_view == 0 )
      return;

   KMenu *menu = m_view->getPopup();

   menu->popup( pos );
}


QSizePolicy MDWEnum::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
}

/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWEnum::nextEnumId() {
   if( m_mixdevice->isEnum() ) {
      int curEnum = enumId();
      if ( curEnum < m_mixdevice->enumValues().count() ) {
         // next enum value
         setEnumId(curEnum+1);
      }
      else {
         // wrap around
         setEnumId(0);
      }
   } // isEnum
}

void MDWEnum::setEnumId(int value)
{
   if (  m_mixdevice->isEnum() ) {
      m_mixdevice->setEnumId( value );
      m_mixdevice->mixer()->commitVolumeChange( m_mixdevice );
   }
}

int MDWEnum::enumId()
{
   if (  m_mixdevice->isEnum() ) {
      return m_mixdevice->enumId();
   }
   else {
      return 0;
   }
}

void MDWEnum::setDisabled()
{
   setDisabled( true );
}

void MDWEnum::setDisabled( bool value ) {
   if ( m_disabled!=value)
   {
      value ? hide() : show();
      m_disabled = value;
   }
}

/**
 * An event filter for the various QWidgets. We watch for Mouse press Events, so
 * that we can popup the context menu.
 */
bool MDWEnum::eventFilter( QObject* obj, QEvent* e )
{
   if (e->type() == QEvent::MouseButtonPress) {
      QMouseEvent *qme = static_cast<QMouseEvent*>(e);
      if (qme->button() == Qt::RightButton) {
         showContextMenu();
         return true;
      }
   } else if (e->type() == QEvent::ContextMenu) {
      QPoint pos = reinterpret_cast<QWidget *>(obj)->mapToGlobal(QPoint(0, 0));
      showContextMenu(pos);
      return true;
   }
    return QWidget::eventFilter(obj,e);
}

#include "mdwenum.moc"
