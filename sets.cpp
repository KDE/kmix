/*
 *              KMix -- KDE's full featured mini mixer
 *
 *
 *              Copyright (C) 1996-2000 Christian Esken
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

#include <kapp.h>
#include <kconfig.h>
#include "sets.h"


MixSetEntry::MixSetEntry()
{
  reset();
}

MixSetEntry::~MixSetEntry()
{
}

void MixSetEntry::reset()
{
  devnum      = 0;
  volumeL     = volumeR  = 0;
  is_disabled = is_muted = false;
  name    = "";
}


void MixSetEntry::read(int /*set*/,int devnum)
{
  QString l_s_token, l_s_value;
  QStrList l_s_MSEs;

  this->devnum = devnum;
  volumeL = volumeR = 50;
  is_disabled = is_muted = false;
  StereoLink = true;

  QString l_s_channelConfig = QString("Channel%1").arg(devnum); // !!! Why does this crash ???
  kapp->config()->readListEntry( l_s_channelConfig, l_s_MSEs, ',');

 
  int l_i_posSeparator;

  if ( l_s_MSEs.count() > 0 ) {
    char *l_cs_mse;
    for ( l_cs_mse = l_s_MSEs.first(); l_cs_mse != 0 ; l_cs_mse = l_s_MSEs.next()) {
      l_s_token = QString(l_cs_mse);
      l_i_posSeparator = l_s_token.find("=");
      if ( l_i_posSeparator == -1) {
	// "=" not found => no value given => remove token
	l_s_token = "";
      }
      else {
	// "=" found => split in real token and value
	l_s_value = l_s_token.mid(l_i_posSeparator+1);
	l_s_token = l_s_token. left(l_i_posSeparator);
      }
    
      // Stupid C++ has only case-construct for simple data types. :-(
      // So I'll do the stupid "if .. else if" stuff.
      if ( l_s_token.left(1) == "L" ) {
	volumeL = l_s_value.toInt();
      }
      else if ( l_s_token.left(1) == "R" ) {
	volumeR = l_s_value.toInt();
      }
      else if ( l_s_token.left(1) == "D" ) {
	is_disabled = l_s_value.toInt();
      }
      else if ( l_s_token.left(1) == "M" ) {
	is_muted = l_s_value.toInt();
      }
      else if ( l_s_token.left(1) == "S" ) {
	StereoLink = l_s_value.toInt();
      }
    }
  }
}


void MixSetEntry::write(int /*set*/,int devnum)
{
  QString l_s_mse;
  
  l_s_mse = QString("L=%1,R=%2,D=%3,M=%4,S=%5").arg(volumeL).arg(volumeR).arg((int)is_disabled).arg((int)is_muted).arg((int)StereoLink);
  kapp->config()->writeEntry( QString("Channel%1").arg(devnum), l_s_mse );
}

void MixSetEntry::clone(MixSetEntry &Src, MixSetEntry &Dest, bool clone_volume)  // bound static
{
  if (clone_volume) {
    Dest.volumeL      = Src.volumeL;
    Dest.volumeR      = Src.volumeR;
    Dest.StereoLink   = Src.StereoLink;
  }
  Dest.is_disabled  = Src.is_disabled;
  Dest.is_muted     = Src.is_muted;
  Dest.devnum       = Src.devnum;
}




MixSet::MixSet()
{
  resize(32);

  for (int i=0; i<32; i++) {
    MixSetEntry* l_mse = new MixSetEntry();
    this->insert(i, l_mse);
  }
}

MixSet::~MixSet()
{
}

void MixSet::read(int set)
{
  QString grp = "Set1";
  //!!!  QString grp = QString("Set%1").arg(set);
  kapp->config()->setGroup(grp);

  for ( unsigned int i=0; i<this->size(); i++ ) {
    MixSetEntry &mse = *(this->operator[](i));
    mse.read(set,i);
  }
}
void MixSet::write(int set)
{
  QString grp = "Set%1";
  grp = grp.arg(set);
  kapp->config() ->setGroup(grp);

  for ( unsigned int i=0; i<this->size(); i++ ) {
    MixSetEntry &mse = *(this->operator[](i));
    mse.write(set,i);
  }
}

void MixSet::clone(MixSet &Src, MixSet &Dest, bool clone_volume) // bound static
{
  for ( unsigned int i=0; i<Src.size(); i++ ) {
    MixSetEntry &src  = *(Src[i]);
    MixSetEntry &dest = *(Dest[i]);
    MixSetEntry::clone(src,dest,clone_volume);
  }
}

MixSetList::MixSetList()
{
  kapp->config()->setGroup(0);
  int NumSets = kapp->config()->readNumEntry( "NumSets"  , 1 );

  for (int i=0; i<NumSets; i++) {
    addSet();
  }
}

MixSetList::~MixSetList()
{
}

void MixSetList::addSet()
{
  resize( size() +1 );
  MixSet *l_ms = new MixSet();
  this->insert(size()-1, l_ms);
}

void MixSetList::read()
{
  return;
  //#warning Something is wrong. Several functons crash in in glibc alloc functions :-(
  for ( unsigned int l_i_setNum=0; l_i_setNum<this->size(); l_i_setNum++ ) {
    MixSet &ms = *(this->operator[](l_i_setNum));
    ms.read(l_i_setNum);
  }
}

void MixSetList::write()
{
  unsigned int l_i_setNum;

  for ( l_i_setNum=0; l_i_setNum<this->size(); l_i_setNum++ ) {
    MixSet &ms = *(this->operator[](l_i_setNum));
    ms.write(l_i_setNum);
  }
  kapp->config()->setGroup(0);
  kapp->config()->writeEntry( "NumSets", l_i_setNum );
}
