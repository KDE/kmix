/*
 *                     The KDE docking class
 *
 *
 *              Copyright (C) 1999 Christian Esken
 *                        esken@kde.org
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <kwm.h>
#include <kapp.h>
#include <klocale.h>

#include "docking.h"
#include <qmessagebox.h>


// --- Constructor ---
KDockWidget::KDockWidget(const char* name, const char* dockPixmapName)
  : QWidget(0, name, 0)
{
  baseInit();
  setPixmap(dockPixmapName);
}

// --- Constructor ---
KDockWidget::KDockWidget(const char* name, QPixmap *dockPixmap)
  : QWidget(0, name, 0)
{
  baseInit();
  setPixmap(dockPixmap);
}


// --- set a new dock Pixmap by filename ---
void KDockWidget::setPixmap(const char* dockPixmapName)
{
  if ( dockPixmapName == 0 ) {
    pm_dockPixmap = 0;
  }
  else {
    pm_dockPixmap = new QPixmap();
    // load dock icon pixmap
    if ( dockPixmapName != 0 && !pm_dockPixmap->load(dockPixmapName) ) {
      QString tmp;
      // !!! this should use KDE error handling functions
      tmp = i18n("Could not load ") + dockPixmapName;
      QMessageBox::warning(this, i18n("Error"), tmp);
    }
  }
}


// --- set a new dock Pixmap by giving a QPixmap ---
void KDockWidget::setPixmap(QPixmap* dockPixmap)
{
  this->pm_dockPixmap = dockPixmap;
}


void KDockWidget::baseInit() {
  // initialize some basic variables
  docked = false;
  pos_x = pos_y = 0;

  // Create standard popup menu for right mouse button
  popup_m = new QPopupMenu();

  // Insert standard item "Restore" into context menu of docking area
  toggleID = popup_m->insertItem(i18n("Restore"),
				 this, SLOT(toggle_window_state()));
  
  // Insert standard item "Quit" into context menu of docking area
  popup_m->insertItem(i18n("Quit"),
		      this, SLOT(emit_quit()));
}


// --- Destructor ---
KDockWidget::~KDockWidget()
{
  delete popup_m;
}


// --- Dock on Panel ---
void KDockWidget::dock()
{
  if (!docked) {
    // Tell the Panel, this widget wants a place on the docking area
    KWM::setDockWindow (this->winId());

    // that's all the space there is !!! SHOULD BE REWORKED (ask kpanel maintainer) !!!
    this->setFixedSize(24, 24);

    // finally show the docking widget on the docking area
    show();
    docked = true;
  }
}


// --- Undock from Panel ---
void KDockWidget::undock()
{
  if (docked) {
    // The widget's window has to be destroyed in order to undock from the
    // panel. Simply using hide() is not enough.
    this->destroy(true, true);

    // recreate docking widget for further dockings
    this->create(0, true, false);
    docked = false;
  }
}


// --- check whether dock widget is in "docked" state ---
bool KDockWidget::isDocked() const
{
  return docked;
}


void KDockWidget::setMainWindow(KTMainWindow *ktmw)
{
  this->ktmw = ktmw;
}



void KDockWidget::paintEvent (QPaintEvent* )
{
  paintIcon();
}

void KDockWidget::paintIcon ()
{
  bitBlt(this, 0, 0, pm_dockPixmap);
}

void KDockWidget::timeclick()
{
  if( this->isVisible() )
    paintIcon();
}


void KDockWidget::mousePressEvent(QMouseEvent *e)
{
  // open/close connect-window on left mouse button 
  if ( e->button() == LeftButton ) {
    toggle_window_state();
  }

  // open popup menu on right mouse button
  if ( e->button() == RightButton  || e->button() == MidButton) {
    int x = e->x() + this->x();
    int y = e->y() + this->y();

    QString text;
    if(ktmw != 0 && ktmw->isVisible())
      text = i18n("&Minimize");
    else
      text = i18n("&Restore");
    
    popup_m->changeItem(text, toggleID);
    popup_m->popup(QPoint(x, y));
    popup_m->exec();
  }

}

void KDockWidget::toggle_window_state()
{
  if(ktmw != 0) {
    if (ktmw->isVisible()) {
      // --- Toplevel was visible => hide it
      dockingInProgress = true;
      ktmw->fullHide();
    }
    else {
      // --- Toplevel was invisible => show it again
      ktmw->fullShow();
      dockingInProgress = false;
    }
  }
}



void KDockWidget::emit_quit()
{
  emit quit_clicked();
}

#include "docking.moc"

