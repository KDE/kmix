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
  tabctl->addTab( page1, "General" );
  tabctl->addTab( page2, "Channels" );
  tabctl->setGeometry( 10, 10, tabwidth, tabheight );

  int x=10,y=20;
  // Define page 1
  QButtonGroup *grpbox1a = new QButtonGroup( "General", page1 );
  grpbox1a->setGeometry( x, y, tabctl->width()-20, tabctl->height()-80 );

  y+=10;
  menubarChk = new QCheckBox(grpbox1a,"Menubar");
  menubarChk->setText("Menubar");
  QFontMetrics qfm = fontMetrics();
  menubarChk->setGeometry(x,y, grpbox1a->width()-20, menubarChk->height() );

  y += (menubarChk->height() );
  tickmarksChk = new QCheckBox(grpbox1a,"Tickmarks");
  tickmarksChk->setText("Tickmarks");
  qfm = fontMetrics();
  tickmarksChk->setGeometry(x,y, grpbox1a->width()-20, tickmarksChk->height() );

  // Define page 2
  updateChannelConfWindow();


  buttonOk = new QPushButton( "Ok", this );
  connect( buttonOk, SIGNAL(clicked()), this, SLOT(slotOk()));

  buttonApply = new QPushButton( "Apply", this );
  connect( buttonApply, SIGNAL(clicked()), this, SLOT(slotApply()));

  buttonCancel = new QPushButton( "Cancel", this );
  connect( buttonCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));

  int maxheight = grpbox1a->height();
  if ( maxheight < grpbox2a->height() )
    maxheight = grpbox2a->height();

  buttonOk->setGeometry( tabwidth-250   , maxheight+73, 80, 25 );
  buttonApply->setGeometry( tabwidth-160, maxheight+73, 80, 25 );
  buttonCancel->setGeometry( tabwidth-70, maxheight+73, 80, 25 );

  grpbox1a->resize(tabwidth-20,maxheight);
  grpbox2a->resize(tabwidth-20,maxheight);
  tabctl->resize(tabwidth,maxheight +60 );
  setFixedSize( tabwidth + 20, maxheight+100 );
  setCaption( "KMix Preferences" );
}


void Preferences::updateChannelConfWindow()
{
  grpbox2a = new QGroupBox ("Mixer channel setup (not saved yet)",page2);
  MixDevice *mdev = mix->First;
  QLabel *qlb;

  int ypos=20;
  int x1=20,x2=130;
  qlb = new QLabel(grpbox2a,"Device");
  qlb->setText("Device");
  qlb->move(x1,ypos);
  qlb = new QLabel(grpbox2a,"Show");
  qlb->setText("Show");
  qlb->move(x2,ypos);

  while (mdev) {
    QLineEdit *qle;
    qle = new  QLineEdit(grpbox2a,mdev->devname);
    qle->setText(mdev->devname);
    ypos += qle->height();
    qle->move(x1,ypos);
    QCheckBox *qcb = new QCheckBox(grpbox2a);
    qcb->move(x2,ypos);
    if (mdev->is_disabled)
      qcb->setChecked(false);
    else
      qcb->setChecked(true);

    mdev = mdev->Next;
  }

  grpbox2a->setGeometry( 10, 20, tabctl->width()-20, ypos+40);
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


