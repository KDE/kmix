///*
// * KMix -- KDE's full featured mini mixer
// *
// *
// * Copyright (C) 2004 Christian Esken <esken@kde.org>
// *
// * This program is free software; you can redistribute it and/or
// * modify it under the terms of the GNU Library General Public
// * License as published by the Free Software Foundation; either
// * version 2 of the License, or (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// * Library General Public License for more details.
// *
// * You should have received a copy of the GNU Library General Public
// * License along with this program; if not, write to the Free
// * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// */
//
//#include <qcursor.h>
//#include <QLabel>
//#include <QMouseEvent>
//#include <QObject>
//
//#include <klocale.h>
//#include <kconfig.h>
//#include <kaction.h>
//#include <kmenu.h>
//#include <kglobalaccel.h>
//#include <kdebug.h>
//#include <ktoggleaction.h>
//#include <kactioncollection.h>
//
//#include "mdwswitch.h"
//#include "core/mixer.h"
//#include "viewbase.h"
//#include "verticaltext.h"
//
///**
// * Class that represents a single Switch
// * The orientation (horizontal, vertical) can be configured
// */
//MDWSwitch::MDWSwitch(MixDevice* md,
//                     bool small, Qt::Orientation orientation,
//                     QWidget* parent, ViewBase* mw) :
//    MixDeviceWidget(md,small,orientation,parent,mw),
//    _label(0) , _labelV(0) , _switchLED(0), _layout(0)
//{
//    // create actions (on _mdwActions, see MixDeviceWidget)
//
//    // KStandardAction::showMenubar() is in MixDeviceWidget now
//    KToggleAction *action = _mdwActions->add<KToggleAction>( "hide" );
//    action->setText( i18n("&Hide") );
//    connect(action, SIGNAL(triggered(bool)), SLOT(setDisabled()));
//    KAction *b = _mdwActions->addAction( "keys" );
//    b->setText( i18n("C&onfigure Shortcuts...") );
//    connect(b, SIGNAL(triggered(bool)), SLOT(defineKeys()));
//
//    // create widgets
//    createWidgets();
//
//    KAction *a = _mdwActions->addAction( "Toggle switch" );
//    a->setText( i18n( "Toggle Switch" ) );
//    connect(a, SIGNAL(triggered(bool)), SLOT(toggleSwitch()));
//
//    // The accel keys are loaded in KMixerWidget::loadConfig, see kmixtoolbox.cpp
//
//    installEventFilter( this ); // filter for popup
//}
//
//MDWSwitch::~MDWSwitch()
//{
//}
//
//
//void MDWSwitch::createWidgets()
//{
//   if ( _orientation == Qt::Vertical ) {
//      _layout = new QVBoxLayout( this );
//      _layout->setAlignment(Qt::AlignHCenter);
//   }
//   else {
//      _layout = new QHBoxLayout( this );
//      _layout->setAlignment(Qt::AlignVCenter);
//   }
//   this->setToolTip( m_mixdevice->readableName() );
//
//
//   _layout->addSpacing( 4 );
//   // --- LEDS --------------------------
//   if ( _orientation == Qt::Vertical ) {
//      if( m_mixdevice->captureVolume().hasSwitch() )
//         _switchLED = new QCheckBox( Qt::red,
//               m_mixdevice->isRecSource()?KLed::On:KLed::Off,
//               KLed::Sunken, KLed::Circular, this, "RecordLED" );
//      else
//         _switchLED = new QCheckBox( Qt::yellow, KLed::On, KLed::Sunken, KLed::Circular, this, "SwitchLED" );
//         _switchLED->setFixedSize(16,16);
//         _labelV = new VerticalText( this, m_mixdevice->readableName().toUtf8().data() );
//
//         _layout->addWidget( _switchLED );
//         _layout->addSpacing( 2 );
//         _layout->addWidget( _labelV );
//
//         _switchLED->installEventFilter( this );
//         _labelV->installEventFilter( this );
//      }
//      else
//      {
//      if( m_mixdevice->captureVolume().hasSwitch() )
//         _switchLED = new QCheckBox( Qt::red,
//               m_mixdevice->isRecSource()?KLed::On:KLed::Off,
//               KLed::Sunken, KLed::Circular, this, "RecordLED" );
//      else
//         _switchLED = new QCheckBox( Qt::yellow, KLed::On, KLed::Sunken, KLed::Circular, this, "SwitchLED" );
//         _switchLED->setFixedSize(16,16);
//         _label  = new QLabel(m_mixdevice->readableName(), this );
//         _label->setObjectName( QLatin1String("SwitchName" ));
//
//         _layout->addWidget( _switchLED );
//         _layout->addSpacing( 1 );
//         _layout->addWidget( _label );
//         _switchLED->installEventFilter( this );
//         _label->installEventFilter( this );
//      }
//      connect( _switchLED, SIGNAL(stateChanged(bool)), this, SLOT(toggleSwitch()) );
//      _layout->addSpacing( 4 );
//}
//
//void MDWSwitch::update()
//{
//   if ( _switchLED != 0 ) {
//      _switchLED->blockSignals( true );
//      if( m_mixdevice->captureVolume().hasSwitch() )
//         _switchLED->setState( m_mixdevice->isRecSource() ? KLed::On : KLed::Off );
//      else
//         _switchLED->setState( m_mixdevice->isMuted() ? KLed::Off : KLed::On );
//
//      _switchLED->blockSignals( false );
//   }
//}
//
//void MDWSwitch::setBackgroundRole(QPalette::ColorRole m)
//{
//   if ( _label != 0 ){
//      _label->setBackgroundRole(m);
//   }
//   if ( _labelV != 0 ){
//      _labelV->setBackgroundRole(m);
//   }
//   _switchLED->setBackgroundRole(m);
//   MixDeviceWidget::setBackgroundRole(m);
//}
//
//void MDWSwitch::showContextMenu()
//{
//   if( m_view == 0 )
//   return;
//
//    KMenu *menu = m_view->getPopup();
//
//    QPoint pos = QCursor::pos();
//    menu->popup( pos );
//}
//
//
//QSizePolicy MDWSwitch::sizePolicy() const
//{
//    if ( _orientation == Qt::Vertical ) {
//        return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::MinimumExpanding );
//    }
//    else {
//        return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
//    }
//}
//
///**
//   This slot is called, when a user has clicked the mute button. Also it is called by any other
//    associated KAction like the context menu.
//*/
//void MDWSwitch::toggleSwitch() {
//   if( m_mixdevice->captureVolume().hasSwitch() )
//      setSwitch( !m_mixdevice->isRecSource() );
//   else
//      setSwitch( !m_mixdevice->isMuted() );
//}
//
//void MDWSwitch::setSwitch(bool value)
//{
//   if (  m_mixdevice->playbackVolume().hasSwitch() ) {
//      if ( m_mixdevice->captureVolume().hasSwitch() ) {
//         m_mixdevice->mixer()->setRecordSource( m_mixdevice->id(), value );
//      }
//      else {
//         m_mixdevice->setMuted( value );
//         m_mixdevice->mixer()->commitVolumeChange( m_mixdevice );
//      }
//   }
//}
//
//void MDWSwitch::setDisabled()
//{
//   setDisabled( true );
//}
//
//void MDWSwitch::setDisabled( bool value ) {
//   if ( m_disabled!=value)
//   {
//      value ? hide() : show();
//      m_disabled = value;
//   }
//}
//
///**
// * An event filter for the various QWidgets. We watch for Mouse press Events, so
// * that we can popup the context menu.
// */
//bool MDWSwitch::eventFilter( QObject* obj, QEvent* e )
//{
//   if (e->type() == QEvent::MouseButtonPress) {
//      QMouseEvent *qme = static_cast<QMouseEvent*>(e);
//      if (qme->button() == Qt::RightButton) {
//            showContextMenu();
//            return true;
//      }
//   }
//   return QWidget::eventFilter(obj,e);
//}
//
//#include "mdwswitch.moc"
