/*
 * KDockWidget
 *
 *              Copyright (C) 1999 Christian Esken
 *                       esken@kde.org
 * 
 * This class is based on a contribution by Harri Porten <porten@tu-harburg.de>
 * It has been reworked by Christian Esken to be a generic base class for
 * docking.
 *
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


#ifndef _DOCKING_H_
#define _DOCKING_H_

#include <ktmainwindow.h>
#include <qpixmap.h>
#include <qpoint.h>


class KDockWidget : public QWidget {

  Q_OBJECT

public:
  KDockWidget(const QString& name=0, const QString& dockIconName=0);
  ~KDockWidget();
  void setMainWindow(KTMainWindow *ktmw);
  QPixmap* dockPixmap() const;
  /// Checks, if application is in "docked" state. Returns true, if yes.
  bool isDocked() const;
  bool isToggled() const;  // !!! Remove?
  void savePosition();

protected:
  void paintEvent(QPaintEvent *e);
  void paintIcon();

private slots:
  void timeclick();
  void toggle_window_state();
  void mousePressEvent(QMouseEvent *e);

public slots:
  virtual void dock();
  virtual void undock();


signals:
  void quit_clicked();

protected slots:
  void emit_quit();

protected:
  bool docked;
  int toggleID;
  int pos_x;
  int pos_y;
  KTMainWindow *ktmw;
  QPopupMenu *popup_m;
  QPixmap dockArea_pixmap;
  bool dockingInProgress;
  bool toggled;  // !!! Remove ???
};

#endif

