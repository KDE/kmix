/*
 * Copyright by Christian Esken 1996-97
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS `AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
                                                                            
#include "prefs.h"
#include "prefs.moc"
#include "kmix.h"
#include <qchkbox.h> 
#include <qlabel.h> 

Preferences::Preferences( QWidget *parent, Mixer *mix ) :
   QDialog( parent )
{
  int tabwidth = 300, tabheight = 200;

  this->mix = mix;
  grpbox2a  = NULL;

  // create the tabbox
  tabctl = new KTabCtl( this );
   
  page1 = new QWidget( tabctl );
  page2 = new QWidget( tabctl );
  tabctl->addTab( page1, "Options" );
  tabctl->addTab( page2, "Sliders" );
  tabctl->setGeometry( 10, 10, tabwidth, tabheight );

  int x=10,y=10;
  // Define page 1
  QButtonGroup *grpbox1a = new QButtonGroup( "General", page1 );
  grpbox1a->setGeometry( 10, 10, tabctl->width()-20, tabctl->height()-80 );

  y+=10;
  menubarChk = new QCheckBox(grpbox1a,"Menubar");
  menubarChk->setText("Menubar");
  QFontMetrics qfm = fontMetrics();
  menubarChk->setGeometry(x,y, grpbox1a->width()-20, menubarChk->height() );

  y += (4 + menubarChk->height() );
  tickmarksChk = new QCheckBox(grpbox1a,"Tickmarks");
  tickmarksChk->setText("Tickmarks");
  qfm = fontMetrics();
  tickmarksChk->setGeometry(x,y, grpbox1a->width()-20, tickmarksChk->height() );

  // Define page 2
  updateChannelConfWindow();


  buttonOk = new QPushButton( "Ok", this );
  buttonOk->setGeometry( tabwidth-250, tabheight+20, 80, 25 );
  connect( buttonOk, SIGNAL(clicked()), this, SLOT(slotOk()));

  buttonApply = new QPushButton( "Apply", this );
  buttonApply->setGeometry( tabwidth-160, tabheight+20, 80, 25 );
  connect( buttonApply, SIGNAL(clicked()), this, SLOT(slotApply()));

  buttonCancel = new QPushButton( "Cancel", this );
  buttonCancel->setGeometry( tabwidth-70, tabheight+20, 80, 25 );
  connect( buttonCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));
   
  setFixedSize( tabwidth + 20, tabheight + 55 );
  setCaption( "KMedia Preferences" );
}


void Preferences::updateChannelConfWindow()
{
  if(grpbox2a)
    delete grpbox2a;
  grpbox2a = new QGridLayout(page1,2,2+mix->num_mixdevs);
  MixDevice *mdev = mix->First;

  int line=1;
  QLabel *qlb;

  qlb = new QLabel("Device");
  grpbox2a->addWidget(qlb,0,0);
  qlb = new QLabel("Show");
  grpbox2a->addWidget(qlb,1,0);

  while (mdev) {
    qlb = new  QLabel(mdev->devname);
    grpbox2a->addWidget((QWidget*)qlb,0,line);
    QCheckBox *qcb = new QCheckBox();
    if (mdev->is_disabled)
      qcb->setChecked(false);
    else
      qcb->setChecked(true);
    grpbox2a->addWidget((QWidget*)qcb,1,line);

    line++;
    mdev = mdev->Next;
  }
  grpbox2a->activate();
}      


void Preferences::slotShow()
{
   show();
}

void Preferences::slotOk()
{
  slotApply();
  hide();
}

void Preferences::slotApply()
{
  emit optionsApply();
}

void Preferences::slotCancel()
{
  hide();
}


