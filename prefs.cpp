/*
 * Copyright by Christian Esken 1996-98
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
#include <klocale.h>
#include <qchkbox.h> 
#include <qlabel.h>

Preferences::Preferences( QWidget *parent, Mixer *mix ) :
   QTabDialog( parent )
{
  resize(300,400);
  this->mix = mix;
  grpbox2a  = NULL;

  page1 = new QWidget( this );
  page1->setGeometry(10,10,width()-20,height()-20);
  page2 = new QWidget( this );
  page2->setGeometry(10,10,width()-20,height()-20);
  addTab( page1, klocale->translate("General") );
  addTab( page2, klocale->translate("Channels") );

  // Define page 1
  QButtonGroup *grpbox1a = new QButtonGroup( klocale->translate("Startup settings"), page1 );
  grpbox1a->setGeometry( 10, 10, page1->width()-20, page1->height()-20 );

  int x=10, y=20;
  menubarChk = new QCheckBox(grpbox1a);
  menubarChk->setText(klocale->translate("Menubar"));
  menubarChk->setGeometry(x,y, grpbox1a->width()-20, menubarChk->height() );

  y += (menubarChk->height() );
  tickmarksChk = new QCheckBox(grpbox1a);
  tickmarksChk->setText(klocale->translate("Tickmarks"));
  tickmarksChk->setGeometry(x,y, grpbox1a->width()-20, tickmarksChk->height() );

  y += tickmarksChk->height();
  grpbox1a->setGeometry( 10, 10, page1->width()-20, y+10);
  // Define page 2
  updateChannelConfWindow();

  setCancelButton();
  setApplyButton();
  setOkButton();


  connect( this, SIGNAL(applyButtonPressed()), this, SLOT(slotApply()));
  connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(slotCancel()));

  int maxheight = grpbox1a->height();
  if ( maxheight < grpbox2a->height() )
    maxheight = grpbox2a->height();

  page1->setFixedSize(page1->width(),maxheight+20);
  page2->setFixedSize(page1->width(),maxheight+20);
  grpbox1a->resize(grpbox1a->width(),maxheight);
  grpbox2a->resize(grpbox2a->width(),maxheight);

  setCaption( klocale->translate("KMix Preferences") );
}


void Preferences::updateChannelConfWindow()
{
  grpbox2a = new QGroupBox (klocale->translate("Mixer channel setup (not saved yet)"),page2);
  MixDevice *mdev = mix->First;
  QLabel *qlb;

  int ypos=20;
  int x1=10,x2=120;
  qlb = new QLabel(grpbox2a);
  qlb->setText(klocale->translate("Device"));
  qlb->move(x1,ypos);
  qlb = new QLabel(grpbox2a);
  qlb->setText(klocale->translate("Show"));
  qlb->move(x2,ypos);
  ypos += qlb->height();

  while (mdev) {

    /// TODO: Create an array, where qle's are inserted. Dann bei "apply"
    /// das qle Array durchgehen und neue Namen setzen.

    QLineEdit *qle;
    qle = new  QLineEdit(grpbox2a,mdev->devname);
    qle->setText(mdev->devname);
    qle->move(x1,ypos);
    QCheckBox *qcb = new QCheckBox(grpbox2a);
    qcb->move(x2,ypos);
    if (mdev->is_disabled)
      qcb->setChecked(false);
    else
      qcb->setChecked(true);

    ypos += qle->height();
    mdev = mdev->Next;
  }

  grpbox2a->setGeometry( 10, 10, page2->width()-20, ypos+10);
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


