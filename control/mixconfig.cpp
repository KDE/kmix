/**
 *  mixconfig.cpp
 *
 *  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <qlayout.h>
#include <qvbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kapp.h>
#include <kfile.h>
#include <qtextstream.h>
#include <kiconloader.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <qwhatsthis.h>
#include <qprogressdialog.h>

#include "mixconfig.h"

KMixConfig::KMixConfig(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  QVBoxLayout *topLayout = new QVBoxLayout(this, 5);

  // Restore settings
  QGroupBox *restGrp = new QGroupBox( i18n("Default Volumes"), this );
  topLayout->addWidget( restGrp );
  QBoxLayout *restLayout =
     new QVBoxLayout( restGrp, KDialog::marginHint(), KDialog::spacingHint());
  restLayout->addSpacing( fontMetrics().lineSpacing() );

  // Save profile
  QHBoxLayout *profLayout = new QHBoxLayout( restLayout, 5 );
  QPushButton *saveProf = new QPushButton( i18n("Save current volumes"), restGrp );
  profLayout->addWidget( saveProf );
  connect( saveProf, SIGNAL(clicked()), this, SLOT(saveVolumes()) );

  // Load profile
  QPushButton *loadProf = new QPushButton( i18n("Load volumes"), restGrp );
  profLayout->addWidget( loadProf );
  connect( loadProf, SIGNAL(clicked()), this, SLOT(loadVolumes()) );

  // start in startkde?
  m_startkdeRestore = new QCheckBox( i18n("Load volumes on login"), restGrp );
  restLayout->addWidget( m_startkdeRestore );
  connect( m_startkdeRestore, SIGNAL(clicked()), this, SLOT(configChanged()) );

  // Hardware settings
  QGroupBox *hdwGrp = new QGroupBox( i18n("Hardware Settings"), this );
  topLayout->addWidget( hdwGrp );
  QBoxLayout *hdwLayout =
     new QVBoxLayout( hdwGrp, KDialog::marginHint(), KDialog::spacingHint());
  hdwLayout->addSpacing( fontMetrics().lineSpacing() );

  m_maxCards = new KIntNumInput( hdwGrp );
  m_maxCards->setLabel( i18n("Maximum number of probed mixers") );
  m_maxCards->setRange( 1, 16 );
  hdwLayout->addWidget( m_maxCards );
  connect( m_maxCards, SIGNAL(valueChanged(int)), this, SLOT(configChanged()) );
  QWhatsThis::add( m_maxCards, i18n("Change this value to optimize the startup time "
                                    "of kmix.\n"
                                    "High values mean that kmix probes for "
                                    "many soundcards. If you have more mixers "
                                    "installed than kmix detects, increase this "
                                    "value.") );

  m_maxDevices = new KIntNumInput( hdwGrp );
  m_maxDevices->setLabel( i18n("Maximum number of probed devices per mixer") );
  m_maxDevices->setRange( 1, 16 );
  hdwLayout->addWidget( m_maxDevices );
  connect( m_maxDevices, SIGNAL(valueChanged(int)), this, SLOT(configChanged()) );
  QWhatsThis::add( m_maxDevices,
                   i18n("Change this value to optimize the startup time "
                        "of kmix. High values mean that kmix probes for "
                        "many devices per soundcard driver.\n"
                        "If there are more mixer sub devices in a "
                        "driver than kmix detects, increase this value") );

  topLayout->addStretch( 1 );

  load();
}

KMixConfig::~KMixConfig() {}

void KMixConfig::configChanged()
{
  emit changed(true);
}

void KMixConfig::loadVolumes()
{
   QProgressDialog progress( i18n("Restoring default volumes"), i18n("Cancel"), 1, this );
   KProcess *ctrl = new KProcess;
   QString ctrlExe = KGlobal::dirs()->findExe("kmixctrl");
   if (!ctrlExe)
   {
      kdDebug() << "can't find kmixctrl" << endl;

      KMessageBox::sorry ( this, i18n("The kmixctrl executable can't be found.") );
      return;
   }

   *ctrl << ctrlExe;
   *ctrl << "--restore";
   ctrl->start();

   while ( ctrl->isRunning() )
   {
      if ( progress.wasCancelled() ) break;
      kapp->processEvents();
   }
   progress.setProgress( 1 );

   delete ctrl;
}

void KMixConfig::saveVolumes()
{
   QProgressDialog progress( i18n("Saving default volumes"), i18n("Cancel"), 1, this );
   KProcess *ctrl = new KProcess;
   QString ctrlExe = KGlobal::dirs()->findExe("kmixctrl");
   if (!ctrlExe)
   {
      kdDebug() << "can't find kmixctrl" << endl;

      KMessageBox::sorry ( this, i18n("The kmixctrl executable can't be found.") );
      return;
   }

   *ctrl << ctrlExe;
   *ctrl << "--save";
   ctrl->start();

   while ( ctrl->isRunning() )
   {
      if ( progress.wasCancelled() ) break;
      kapp->processEvents();
   }
   progress.setProgress( 1 );

   delete ctrl;
}

void KMixConfig::load()
{
  KConfig *config = new KConfig("kcmkmixrc", true);

  config->setGroup("Misc");
  m_startkdeRestore->setChecked( config->readBoolEntry( "startkdeRestore", true ) );
  m_maxCards->setValue( config->readNumEntry( "maxCards", 2 ) );
  m_maxDevices->setValue( config->readNumEntry( "maxDevices", 2 ) );
  delete config;

  emit changed(false);
}

void KMixConfig::save()
{

  KConfig *config= new KConfig("kcmkmixrc", false);

  config->setGroup("Misc");
  config->writeEntry( "startkdeRestore", m_startkdeRestore->isChecked() );
  config->writeEntry( "maxCards", m_maxCards->value() );
  config->writeEntry( "maxDevices", m_maxDevices->value() );
  config->sync();
  delete config;

  emit changed(false);
}

void KMixConfig::defaults()
{
  m_startkdeRestore->setChecked( true );
  m_maxCards->setValue( 2 );
  m_maxDevices->setValue( 2 );
  emit changed(true);
}

extern "C"
{
    KCModule *create_kmix(QWidget *parent, const char *name)
    {
      KGlobal::locale()->insertCatalogue("kcmkmix");
      return new KMixConfig(parent, name);
    }

    void init_kmix()
    {
        KConfig *config = new KConfig("kcmkmixrc", true, false);

        config->setGroup("Misc");
        bool start = config->readBoolEntry( "startkdeRestore", true );
        delete config;

        if ( start )
            kapp->startServiceByDesktopName( "kmixctrl_restore" );
    }
}

#include "mixconfig.moc"
