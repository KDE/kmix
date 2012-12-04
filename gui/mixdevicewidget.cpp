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

#include "gui/mixdevicewidget.h"

#include <kactioncollection.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kaction.h>
#include <kmenu.h>
#include <kglobalaccel.h>
#include <kshortcutsdialog.h>
#include <kdebug.h>

#include <qcursor.h>
#include <QMouseEvent>
#include <qpixmap.h>
#include <qwmatrix.h>

#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "viewbase.h"
#include "ksmallslider.h"
#include "verticaltext.h"

/**
 * Base Class for any Widget that represents a MixDevice.
 * The mix device can be a real (hardware bound) MixDevice or a virtual mix device.
 *
 * The direction (horizontal, vertical) can be configured and whether it should
 * be "small"  (uses KSmallSlider instead of a normal slider widget). The actual implementations
 * SHOULD honor these values - those who do not might not be suitable for placing in
 * the panel applet or any other smallish settings.
 */
MixDeviceWidget::MixDeviceWidget(shared_ptr<MixDevice> md,
                                 bool small, Qt::Orientation orientation,
                                 QWidget* parent, ViewBase* view, ProfControl* par_pctl) :
   QWidget( parent ), m_mixdevice( md ), m_view( view ), _pctl(par_pctl),
   m_disabled( false ), _orientation( orientation ), m_small( small )
   , m_shortcutsDialog(0)
{
   _mdwActions = new KActionCollection( this );
   _mdwPopupActions = new KActionCollection( this );
   
   QString name (md->id());
  /* char* whatsThisChar = whatsthis.toUtf8().data();
   QString w;
   w = ki18n(whatsThisChar).toString(MixerToolBox::whatsthisControlLocale() );
   this->setWhatsThis(w);
   */
   QString whatsthisText = mixDevice()->mixer()->translateKernelToWhatsthis(name);
   if ( whatsthisText != "---") {
      setWhatsThis(whatsthisText);
   }
}

MixDeviceWidget::~MixDeviceWidget()
{
}

void MixDeviceWidget::addActionToPopup( KAction *action )
{
   _mdwActions->addAction( action->objectName(), action );
}

bool MixDeviceWidget::isDisabled() const
{
   return m_disabled;
}

void MixDeviceWidget::defineKeys()
{
   // Dialog for *global* shortcuts of this MDW
   if ( m_shortcutsDialog == 0 ) {
      m_shortcutsDialog = new KShortcutsDialog( KShortcutsEditor::GlobalAction );
      m_shortcutsDialog->addCollection(_mdwPopupActions);
   }
   m_shortcutsDialog->configure();
}

void MixDeviceWidget::volumeChange( int ) { /* is virtual */ }
void MixDeviceWidget::setDisabled( bool ) { /* is virtual */ }
//void MixDeviceWidget::setVolume( int /*channel*/, int /*vol*/ ) { /* is virtual */ }
//void MixDeviceWidget::setVolume( Volume /*vol*/ ) { /* is virtual */ }
void MixDeviceWidget::update() { /* is virtual */ }
void MixDeviceWidget::showContextMenu( const QPoint &pos ) { /* is virtual */ }
void MixDeviceWidget::setColors( QColor , QColor , QColor ) { /* is virtual */ }
void MixDeviceWidget::setIcons( bool ) { /* is virtual */ }
void MixDeviceWidget::setLabeled( bool ) { /* is virtual */ }
void MixDeviceWidget::setMutedColors( QColor , QColor , QColor ) { /* is virtual */ }



void MixDeviceWidget::mousePressEvent( QMouseEvent *e )
{
   if ( e->button() == Qt::RightButton )
      showContextMenu();
   else {
       QWidget::mousePressEvent(e);
   }
}


#include "mixdevicewidget.moc"
