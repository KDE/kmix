// -*-C++-*-
#ifndef KCONTEXTMENU_H
#define KCONTEXTMENU_H

#include <qobject.h>
#include <qpopmenu.h>
#include <qlist.h>

typedef QPopupMenu* (KCmFunc)(QObject *,QObject*);

class KContextMenu
{
public:
  KContextMenu(QObject *o, KCmFunc *f );
  QObject		*o;
  KCmFunc		*f;
};

/** This class manages all ContextMenus of one application. It is called by the event filter
  of the KApplication class. It is a derivation of QObject, so that an event filter may be
  intalled for it.
  */

class KCmManager : QObject
{
public:
  /// Create a ContextMenu manager. You must give a filter object o on creation.
  KCmManager(QObject *o);

  /// Insert an item in the ContextMenu manager by giving object and menu creating function
  void insert(QObject *o, KCmFunc f);
  /// Insert an item in the ContextMenu manager by giving a KContextMenu
  void insert(KContextMenu *kcm);
  void remove(KContextMenu *kcm);
  /** In the eventFilter() of ones application, one can call this function. If you have registered
      a context menu for object o, it will return this function for you to call. */      
  KCmFunc* getContextFunction(QObject *o, QEvent *e);
  
  bool showContextMenu(QPopupMenu *qpm);

private:
  /// This list holds all registered context menus
  QList<KContextMenu>	*CmList;
  /** The filter object is the object, where the event filter is installed. Must be a QObject or
      a derivation. In the initialization of a KApplication you would typically hand over "this". */
  QObject		*filterObj;
  /// The event filter will filter all events for filterObj
  bool eventFilter(QObject *, QEvent *);
  /// This is where the menu will be popped up
  QPoint		popup_point;
};
#endif
