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

#include <kconfig.h>
#include "sets.h"

extern KConfig	     *KmConfig;


MixSetEntry::MixSetEntry()
{
  devnum      = 0;
  volumeL     = volumeR  = 0;
  is_disabled = is_muted = false;
  name    = "";
}
MixSetEntry::~MixSetEntry()
{
}
void MixSetEntry::read(int set,int devnum)
{
  QString grp;
  grp.sprintf("Set%i.Dev%i",set,devnum);
  KmConfig->setGroup(grp);
  this->devnum = devnum;
  volumeL      = KmConfig->readNumEntry("volumeL", 50);
  volumeR      = KmConfig->readNumEntry("volumeR", 50);
  is_disabled  = KmConfig->readNumEntry("is_disabled", 0);
  is_muted     = KmConfig->readNumEntry("is_muted", 0);
  StereoLink   = KmConfig->readNumEntry("StereoLink", 1);
  name         = KmConfig->readEntry("name", "unnamed");
}
void MixSetEntry::write(int set,int devnum)
{
  QString grp;
  grp.sprintf("Set%i.Dev%i",set,devnum);
  KmConfig->setGroup(grp);
  KmConfig->writeEntry("volumeL", volumeL);
  KmConfig->writeEntry("volumeR", volumeR);
  KmConfig->writeEntry("is_disabled", is_disabled);
  KmConfig->writeEntry("is_muted", is_muted);
  KmConfig->writeEntry("StereoLink", StereoLink);
  KmConfig->writeEntry("name", name);
}
void MixSetEntry::clone(MixSetEntry *Src, MixSetEntry *Dest, bool clone_volume)  // bound static
{
  if (clone_volume) {
    Dest->volumeL      = Src->volumeL;
    Dest->volumeR      = Src->volumeR;
    Dest->StereoLink   = Src->StereoLink;
  }
  Dest->is_disabled  = Src->is_disabled;
  Dest->is_muted     = Src->is_muted;
  Dest->name         = Src->name;
}




MixSet::MixSet()
{
  for (int i=0; i<32; i++)
    append( new MixSetEntry );
}
MixSet::~MixSet()
{
}
void MixSet::read(int set)
{
  int i=0;
  for ( MixSetEntry *mse=first(); mse!=NULL; mse=next(), i++ )
    mse->read(set,i);
}
void MixSet::write(int set)
{
  int i=0;
  for ( MixSetEntry *mse=first(); mse!=NULL; mse=next(), i++ )
    mse->write(set,i);
}
void MixSet::clone(MixSet *Src, MixSet *Dest, bool clone_volume) // bound static
{
  MixSetEntry *src,*dest;
  for ( src=Src->first(), dest=Dest->first();
	(src!=0) && (dest!=0);
	src=Src->next(), dest=Dest->next() )
    MixSetEntry::clone(src,dest,clone_volume);
}
MixSetEntry* MixSet::findDev(int num)
{
  MixSetEntry* mse;

  for (mse = first();
       (mse != NULL) && (mse->devnum != num);
       mse=next() );

  return mse;
}





MixSetList::MixSetList()
{
  KmConfig->setGroup("");
  int NumSets = KmConfig->readNumEntry( "NumSets"  , 1 );
  // create one extra set. The first one is the current mix set
  for (int i=0; i<NumSets+1; i++)
    append( new MixSet );
}
MixSetList::~MixSetList()
{
}
MixSet* MixSetList::addSet()
{
  MixSet *m = new MixSet;
  append( m );
  return m;
}

void MixSetList::read()
{
  int i=0;
  for ( MixSet *ms=first(); ms!=NULL; ms=next(),i++ )
    ms->read(i);
}
void MixSetList::write()
{
  int i=1;
  MixSet *ms=first();
  ms=next(); // skip first set (Set 0)

  for ( ; ms!=NULL; ms=next(),i++ )
    ms->write(i);
  KmConfig->setGroup("");
  KmConfig->writeEntry( "NumSets", i-1 );
}



