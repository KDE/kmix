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
