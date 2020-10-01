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
#include <kconfig.h>
#include <kshortcutsdialog.h>
#include <klocalizedstring.h>

#include <qmenu.h>
#include <qevent.h>

#include "core/mixer.h"
#include "core/mixertoolbox.h"
#include "viewbase.h"
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
MixDeviceWidget::MixDeviceWidget(shared_ptr<MixDevice> md, MDWFlags flags, ViewBase *view, ProfControl *pctl)
    : QWidget(view),
      m_flags(flags),
      m_mixdevice(md),
      m_view(view)
{
   setContextMenuPolicy(Qt::DefaultContextMenu);

   // The control profile.  ViewSliders uses the default from the MixDevice.
   // ViewDockAreaPopup sets a special one.
   m_pctl = pctl;
   if (m_pctl==nullptr) m_pctl = md->controlProfile();
   Q_ASSERT(m_pctl!=nullptr);

   m_channelActions = new KActionCollection(this);
   m_globalActions = new KActionCollection(this);
   m_shortcutsDialog = nullptr;
   
   QAction *act = m_channelActions->addAction("keys");
   act->setText(i18n("Channel Shortcuts..."));
   connect(act, &QAction::triggered, this, &MixDeviceWidget::configureShortcuts);

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


void MixDeviceWidget::configureShortcuts()
{
    // Dialog for *global* shortcuts of this MDW
    if (m_shortcutsDialog==nullptr)
    {
        m_shortcutsDialog = new KShortcutsDialog(KShortcutsEditor::GlobalAction);
        m_shortcutsDialog->addCollection(m_globalActions);
    }

    m_shortcutsDialog->configure();
}


void MixDeviceWidget::contextMenuEvent(QContextMenuEvent *ev)
{
    if (view()==nullptr) return;
    QMenu *menu = view()->getPopup();
    menu->addSection(QIcon::fromTheme("kmix"), mixDevice()->readableName());

    createContextMenu(menu);

    // The common "Channel Shortcuts" action
    QAction *act = m_channelActions->action("keys");
    if (act!=nullptr && !m_globalActions->isEmpty())	// action is available, and
    {							// there are shortcuts to define
        menu->addSeparator();
        menu->addAction(act);
    }

    menu->popup(ev->globalPos());
}


void MixDeviceWidget::volumeChange( int ) { /* is virtual */ }
//void MixDeviceWidget::update() { /* is pure virtual */ }
//void MixDeviceWidget::createContextMenu(QMenu *menu) { /* is pure virtual */ }
void MixDeviceWidget::setColors( QColor , QColor , QColor ) { /* is virtual */ }
void MixDeviceWidget::setIcons( bool ) { /* is virtual */ }
void MixDeviceWidget::setLabeled( bool ) { /* is virtual */ }
void MixDeviceWidget::setMutedColors( QColor , QColor , QColor ) { /* is virtual */ }
