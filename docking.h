/*
 * KDockWidget
 *
 *              Copyright (C) 1999 Christian Esken
 *                       esken@kde.org
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

class KTMainWindow;
#include <qpixmap.h>

/**
 This class creates a widget that allows applications to display a widget
 on the docking area of the panel. The following services are provided:
 A popup menu with some standard entries is created. You can
 place further entries in there. The menu is being shown by a right-button
 click on the widget.
 Hide and show a main window  
 */
class KDockWidget : public QWidget {

  Q_OBJECT

public:
  /// Creates a docking widget and allows passing of the name of
  /// the docking icon. You must either pass a valid icon or 0.
  /// Passing 0, you can paint anything you like on the docking widget
  /// by overrdiding tjhe paint event.
  KDockWidget(const QString& dockIcon, const char *name=0);
  /// Overloaded constructor. Only differs from the previous constructor
  /// in that you can pass the icon as a QPixmap
  KDockWidget(const QPixmap& dockPixmap, const char *name=0);
  /// Deletes the docking widget. Please note that the widget undocks from
  /// the panel automatically.
  virtual ~KDockWidget();
  virtual void setMainWindow(KTMainWindow *ktmw);
  /** Checks, if application is in "docked" state. Returns true, if yes.
    It has still to be defined what the exact semantics are: Complete
    application docked, a single window docked, or is this application
    dependent?!? */
  virtual bool isDocked() const;
  //  void savePosition();
  virtual void setPixmap(const QString& dockPixmapName);
  virtual void setPixmap(const QPixmap& dockPixmap);

public slots:
  /// Dock this widget - this means to show this widget on the docking area
  virtual void dock();
  /// Undock this widget - this means to remove this widget from the docking area
  virtual void undock();
  /// A MousePressEvent for the left button. The default implementation shows and
  /// hides the main window set via setMainWindow().
  virtual void mousePressLeftEvent(QMouseEvent *e);
  /// A MousePressEvent for the right button. The default implementation pops up
  /// a menu.
  virtual void mousePressRightEvent(QMouseEvent *e);
  virtual void toggle_window_state();


protected:
  /// The paint event normally repaints the icon by calling paintIcon().
  /// If you do not want to show an icon, but draw some grapics, you
  /// must derive from QDockWidget and override this function.
  void paintEvent(QPaintEvent *e);
  /// Paint the icon.
  virtual void paintIcon();
  virtual void setShowHideText(); 
  virtual void baseInit();

private slots:
  void mousePressEvent(QMouseEvent *e);


signals:
  void quit_clicked();

protected slots:
  void emit_quit();

protected:
  bool		docked;
  int		pos_x;
  int		pos_y;
  int		showHidePopmenuEntry;
  KTMainWindow	*ktmw;
  QPopupMenu	*popup_m;
  QPixmap	pm_dockPixmap;
 };

#endif

