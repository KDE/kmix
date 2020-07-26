/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 2004 Christian Esken <esken@kde.org>
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


// KMix
#include "mdwenum.h"
#include "viewbase.h"
#include "core/mixer.h"

// KDE
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <ktoggleaction.h>

// Qt
#include <QCursor>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QObject>
#include <QBoxLayout>
#include <QComboBox>

/**
 * Class that represents an Enum element (a select one-from-many selector)
 * The orientation (horizontal, vertical) is ignored
 */
MDWEnum::MDWEnum(shared_ptr<MixDevice> md, MixDeviceWidget::MDWFlags flags, ViewBase *view, ProfControl *pctl)
    : MixDeviceWidget(md, flags, view, pctl),
      _label(nullptr),
      _enumCombo(nullptr)
{
    createWidgets();
}


void MDWEnum::createWidgets()
{
    QBoxLayout *_layout;
    if (orientation()==Qt::Vertical)
    {
        _layout = new QVBoxLayout(this);
        _layout->setAlignment(Qt::AlignLeft|Qt::AlignTop);
    }
    else
    {
        _layout = new QHBoxLayout(this);
        _layout->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    }

   _label = new QLabel( mixDevice()->readableName(), this);
   _layout->addWidget(_label);

    if (orientation()==Qt::Horizontal) _layout->addSpacing(8);

   _enumCombo = new QComboBox(this);
   _enumCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

   // ------------ fill ComboBox start ------------
   const QStringList &values = mixDevice()->enumValues();
   int maxEnumId = values.count();
   for (int i=0; i<maxEnumId; i++ ) {
      _enumCombo->addItem(values.at(i));
   }
   // ------------ fill ComboBox end --------------
   _layout->addWidget(_enumCombo);
   connect( _enumCombo, SIGNAL(activated(int)), this, SLOT(setEnumId(int)) );
   _enumCombo->setToolTip( mixDevice()->readableName() );
	_layout->addStretch(1);
}

void MDWEnum::update()
{
  if ( mixDevice()->isEnum() ) {
    //qCDebug(KMIX_LOG) << "MDWEnum::update() enumID=" << mixDevice()->enumId();
    _enumCombo->setCurrentIndex( mixDevice()->enumId() );
  }
  else {
    qCCritical(KMIX_LOG) << "MDWEnum::update() enumID=" << mixDevice()->enumId() << " is no Enum ... skipped";
  }
}


void MDWEnum::createContextMenu(QMenu *menu)
{
    // nothing special for this sort of control
}


QSizePolicy MDWEnum::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
}

/**
   This slot is called, when a user has clicked the mute button. Also it is called by any other
    associated KAction like the context menu.
*/
void MDWEnum::nextEnumId() {
   if( mixDevice()->isEnum() ) {
      int curEnum = enumId();
      if ( curEnum < mixDevice()->enumValues().count() ) {
         // next enum value
         setEnumId(curEnum+1);
      }
      else {
         // wrap around
         setEnumId(0);
      }
   } // isEnum
}

void MDWEnum::setEnumId(int value)
{
   if (  mixDevice()->isEnum() ) {
      mixDevice()->setEnumId( value );
      mixDevice()->mixer()->commitVolumeChange( mixDevice() );
   }
}

int MDWEnum::enumId()
{
   if (  mixDevice()->isEnum() ) {
      return mixDevice()->enumId();
   }
   else {
      return 0;
   }
}


/**
 * For users of this class who would like to show multiple MDWEnum's properly aligned.
 * It returns the size of the control label (in the control layout direction).
 */
int MDWEnum::labelExtentHint() const
{
	if (_label==nullptr) return (0);

	if (orientation()==Qt::Vertical) return (_label->sizeHint().height());
	else return (_label->sizeHint().width());
}

/**
 * If a label from another switch is larger than ours, then the
 * extent of our label is adjusted.
 */
void MDWEnum::setLabelExtent(int extent)
{
	if (_label==nullptr) return;

	if (orientation()==Qt::Vertical) _label->setMinimumHeight(extent);
	else _label->setMinimumWidth(extent);
}
