/**
 * mixconfig.h
 *
 * Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __mixconfig_h__
#define __mixconfig_h__

#include <kcmodule.h>

class QCheckBox;
class KIntNumInput;

class KMixConfig : public KCModule
{
  Q_OBJECT

public:
  KMixConfig(QWidget *parent = 0L, const char *name = 0L);
  virtual ~KMixConfig();
  
  void load();
  void save();
  void defaults();
  
  int buttons();
  
protected slots:
  void configChanged();
  void loadVolumes();
  void saveVolumes();
      
private:
  QCheckBox *m_startkdeRestore;
  KIntNumInput *m_maxDevices;
};

#endif
