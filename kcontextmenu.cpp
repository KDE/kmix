#include "kcontextmenu.h"
#include <iostream.h>


KContextMenu::KContextMenu(QObject *o, KCmFunc *f )
{
  this->o = o;
  this->f = f;
}

KCmManager::KCmManager(QObject *o)
{
  // Clear the ContextMenu list
  CmList = new QList<KContextMenu>;
  filterObj = o;
};

void KCmManager::insert(QObject *o, KCmFunc *f)
{
  const KContextMenu *kcm;

  kcm = new KContextMenu (o, f);
  CHECK_PTR(kcm);
  CmList->inSort((KContextMenu* const) kcm);
  o->installEventFilter(filterObj);
}


void KCmManager::insert(KContextMenu *kcm)
{
  CmList->inSort(  (KContextMenu* const) kcm );
}

void KCmManager::remove(KContextMenu* kcm)
{
  if (! CmList->remove((KContextMenu* const) kcm) )
    cerr << "Trying to remove non-existent ContextMenu " << kcm << " from manager " << this ;
}




KCmFunc* KCmManager::getContextFunction(QObject *o, QEvent *e)
{
  KContextMenu *kcm = NULL;

  // Lets see, if we have a "Right mouse button press"
  if (e->type() == Event_MouseButtonPress)
    {
        QMouseEvent *qme = (QMouseEvent*)e;
      if (qme->button() == RightButton)
	{
	  // Yes. Lets see, if we have to popup something here
	  for (kcm=CmList->first(); kcm!=NULL; kcm=CmList->next() )
	    if (kcm->o == o)
	      break;
	  // QPoint p1 =  qme->pos();
	  // cerr << "Right Mouse button pressed at (" << p1.x() << "," << p1.y() << ").\n";
	  if (kcm)
	    popup_point = QCursor::pos();
	}
    }

  if (kcm)
    return kcm->f;
  else
    return NULL;
}
bool KCmManager::eventFilter(QObject *o, QEvent *e)
{
  bool b;

  KCmFunc *f = getContextFunction(o,e);
  if (f != NULL) {
    QPopupMenu *qpm = f(filterObj,o);
    b = showContextMenu(qpm);
    return b;
  }
  else
    return false;

}
/// The following function must be called by event filter of the KApplication
bool KCmManager::showContextMenu(QPopupMenu *qpm)
{
  if(!qpm)
    return false;
  else			// Ignoring event
    qpm->popup(popup_point);
  return true;		// I eat the event

}
