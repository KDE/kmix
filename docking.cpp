/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-98 Christian Esken
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

#include "kmix.h"

#include <qtooltip.h>
#include <kwm.h>
#include <kapp.h>

#include "docking.h"

extern KApplication *globalKapp;
extern KMix *kmix;
extern bool dockinginprogress;

DockWidget::DockWidget(const char *name)
  : QWidget(0, name, 0) {

  docked = false;

  pos_x = pos_y = 0;
  have_position = false;

  QString pixdir = globalKapp->kde_datadir() + "/kmix/pics/";
  QString tmp;

#define PMERROR(pm) \
  tmp.sprintf(i18n("Could not load %s !"), pm); \
  QMessageBox::warning(this, i18n("Error"), tmp);

  // load pixmaps

  if (!small_pixmap.load(pixdir + "kmixdocked.xpm")){
    PMERROR("kmixdocked.xpm");
  }

  // popup menu for right mouse button
  popup_m = new QPopupMenu();

  toggleID = popup_m->insertItem(i18n("Restore"),
				 this, SLOT(toggle_window_state()));
  
  popup_m->insertItem(i18n("Quit"),
		      this, SLOT(emit_quit()));


  //  QToolTip::add( this, statstring.data() );

}

DockWidget::~DockWidget() {
}

void DockWidget::dock() {

  if (!docked) {


    // prepare panel to accept this widget
    KWM::setDockWindow (this->winId());

    // that's all the space there is
    this->setFixedSize(24, 24);

    // finally dock the widget
    show();
    docked = true;
  }
  if(kmix){
    QPoint point = kmix->mapToGlobal (QPoint (0,0));
    pos_x = point.x();
    pos_y = point.y();

  }
}

void DockWidget::undock() {

  if (docked) {

    // the widget's window has to be destroyed in order 
    // to undock from the panel. Simply using hide() is
    // not enough.
    this->destroy(true, true);

    // recreate window for further dockings
    this->create(0, true, false);

    docked = false;
  }
}

const bool DockWidget::isDocked() {

  return docked;

}

void DockWidget::paintEvent (QPaintEvent *e) {

  (void) e;

  paintIcon();

}

void DockWidget::paintIcon () {

  bitBlt(this, 0, 0, &small_pixmap);


}

void DockWidget::timeclick() {

  if(this->isVisible()){
    paintIcon();
  }  
}


void DockWidget::mousePressEvent(QMouseEvent *e) {

  // open/close connect-window on right mouse button 
  if ( e->button() == LeftButton ) {
    toggle_window_state();
  }

  // open popup menu on left mouse button
  if ( e->button() == RightButton  || e->button() == MidButton) {
    int x = e->x() + this->x();
    int y = e->y() + this->y();

    QString text;
    if(kmix->isVisible())
      text = i18n("Minimize");
    else
      text = i18n("Restore");
    
    popup_m->changeItem(text, toggleID);
    popup_m->popup(QPoint(x, y));
    popup_m->exec();
  }

}

void DockWidget::toggle_window_state() {

    // restore/hide connect-window
    /*
    if(kmix != 0L)  {
        if (kmix->isVisible()){

            QPoint point = kmix->mapToGlobal (QPoint (0,0));
            pos_x = point.x();
            pos_y = point.y();
            toggled = true;
            kmix->hide();
        }
        else {
            if(!have_position)
                savePosition();
            kmix->setGeometry( pos_x,
                               pos_y,
                               kmix->width(),
                               kmix->height());

            toggled = false;
            kmix->show();
        }
    }
    /**/
    if(kmix != 0L)  {
        if (kmix->isVisible()){
            dockinginprogress = true;
            savePosition();
//            QPoint point = k->mapToGlobal (QPoint (0,0));
//            pos_x = point.x();
//            pos_y = point.y();
            toggled = true;
            kmix->hide();
        }
        else {

            if(!have_position)
                savePosition();

            kmix->setGeometry(
                           pos_x,
                           pos_y,
                           kmix->width(),
                           kmix->height());
            toggled = false;
            //
            QPoint zp = kmix->mapToGlobal(QPoint (0,0));
            if(zp.x() == pos_x && zp.y() == pos_y){
//                if(debugflag)
                    printf("warning: qt bug? compensating.....\n");
                kmix->setGeometry(pos_x-4, pos_y-24, kmix->width(), kmix->height());
            }
            //
            kmix->show();
            dockinginprogress = false;
        }
    }

}

const bool DockWidget::isToggled()
{
    return(toggled);
}

void DockWidget::savePosition() {
  QPoint point = kmix->mapToGlobal (QPoint (0,0));
  pos_x = point.x();
  pos_y = point.y();
  have_position = true;
}

void DockWidget::emit_quit()
{
  emit quit_clicked();
}

#include "docking.moc"

