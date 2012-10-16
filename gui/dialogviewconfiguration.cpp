/*
 * KMix -- KDE's full featured mini mixer
 *
 *
 * Copyright (C) 1996-2004 Christian Esken <esken@kde.org>
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

#include "dialogviewconfiguration.h"

#include <algorithm>

#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>

#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>

#include "gui/guiprofile.h"
#include "gui/mixdevicewidget.h"
#include "core/ControlManager.h"
#include "core/mixdevice.h"
#include "core/mixer.h"

DialogViewConfigurationItem::DialogViewConfigurationItem(QListWidget *parent) :
  QListWidgetItem(parent)
{
   kDebug() << "DialogViewConfigurationItem() default constructor";
   refreshItem();
}



DialogViewConfigurationItem::DialogViewConfigurationItem(QListWidget *parent, QString id, bool shown, QString name, int splitted, const QString& iconName) :
   QListWidgetItem(parent), _id(id), _shown(shown), _name(name), _splitted(splitted), _iconName(iconName)
{
  refreshItem();
}

void DialogViewConfigurationItem::refreshItem()
{
  setFlags((flags() | Qt::ItemIsDragEnabled) &  ~Qt::ItemIsDropEnabled);
  setText(_name);
  setIcon(KIconLoader::global()->loadIcon( _iconName, KIconLoader::Small, KIconLoader::SizeSmallMedium ) );
  setData(Qt::ToolTipRole, _id);  // a hack. I am giving up to do it right
  setData(Qt::DisplayRole, _name);
}

static QDataStream & operator<< ( QDataStream & s, const DialogViewConfigurationItem & item ) {
    s << item._id;
    s << item._shown;
    s << item._name;
    s << item._splitted;
    s << item._iconName;
 //kDebug() << "<< unserialize << " << s;
    return s;
}
static QDataStream & operator>> ( QDataStream & s, DialogViewConfigurationItem & item ) {
  QString id;
  s >> id;
  item._id = id;
  bool shown;
  s >> shown;
  item._shown = shown;
  QString name;
  s >> name;
  item._name = name;
  int splitted;
  s >> splitted;
  item._splitted = splitted;
  QString iconName;
  s >> iconName;
  item._iconName = iconName;
 //kDebug() << ">> serialize >> " << id << name << iconName;
  return s;
}

DialogViewConfigurationWidget::DialogViewConfigurationWidget(QWidget *parent)
    : QListWidget(parent),
      m_activeList(true)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setAcceptDrops(true);
setSelectionMode(QAbstractItemView::SingleSelection);
setDragEnabled(true);
viewport()->setAcceptDrops(true);
setAlternatingRowColors(true);

}



QMimeData* DialogViewConfigurationWidget::mimeData(const QList<QListWidgetItem*> items) const
{
    if (items.isEmpty())
        return 0;
    QMimeData* mimedata = new QMimeData();

    DialogViewConfigurationItem* item = 0;
    QByteArray data;
    {
        QDataStream stream(&data, QIODevice::WriteOnly);
        // we only support single selection
        item = static_cast<DialogViewConfigurationItem *>(items.first());
        stream << *item;
    }

    bool active = isActiveList();
    mimedata->setData("application/x-kde-action-list", data);
    mimedata->setData("application/x-kde-source-treewidget", active ? "active" : "inactive");

    return mimedata;
}

bool DialogViewConfigurationWidget::dropMimeData(int index, const QMimeData * mimeData, Qt::DropAction /*action*/)
{
    const QByteArray data = mimeData->data("application/x-kde-action-list");
    if (data.isEmpty())
        return false;
    QDataStream stream(data);
    const bool sourceIsActiveList = mimeData->data("application/x-kde-source-treewidget") == "active";

    DialogViewConfigurationItem* item = new DialogViewConfigurationItem(0); // needs parent, use this temporarily
    stream >> *item;
    item->refreshItem();
    emit dropped(this, index, item, sourceIsActiveList);
    return true;
}

DialogViewConfiguration::DialogViewConfiguration( QWidget*, ViewBase& view)
    : KDialog(  0),
      _view(view)
{
   setCaption( i18n( "Configure Channels" ) );
   setButtons( Ok|Cancel );
   setDefaultButton( Ok );
   frame = new QWidget( this );
   frame->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
   
   setMainWidget( frame );
   
   // The _layout will hold two items: The title and the Drag-n-Drop area
   _layout = new QVBoxLayout(frame );
   _layout->setMargin( 0 );
   _layout->setSpacing(KDialog::spacingHint());
   
   // --- HEADER ---
   qlb = new QLabel( i18n("Configuration of the channels. Drag icon to update."), frame );
   _layout->addWidget(qlb);
   
   _glayout = new QGridLayout();
   _layout->addLayout(_glayout);

   _qlw = 0;
   _qlwInactive = 0;
   createPage();
}



/**
 * Drop an item from one list to the other
 */
void DialogViewConfiguration::slotDropped ( DialogViewConfigurationWidget* list, int index, DialogViewConfigurationItem* item, bool sourceIsActiveList )
{
//kDebug() << "dropped item (index" << index << "): " << item->_id << item->_shown << item->_name << item->_splitted << item->_iconName;

    if ( list == _qlw ) {
        //DialogViewConfigurationItem* after = index > 0 ? static_cast<DialogViewConfigurationItem *>(list->item(index-1)) : 0;

        //kDebug() << "after" << after->text() << after->internalTag();
        if ( sourceIsActiveList ) {
            // has been dragged within the active list (moved).
            _qlw->insertItem ( index, item );
            //moveActive(item, after);
        } else {
            // dragged from the inactive list to the active list
            _qlw->insertItem ( index, item );
            //insertActive(item, after, true);
        }
    }
    else if ( list == _qlwInactive ) {
        // has been dragged to the inactive list -> remove from the active list.
        //removeActive(item);
        _qlwInactive->insertItem ( index, item );
    }

}


void DialogViewConfiguration::addSpacer(int row, int col)
{
	QWidget *dummy = new QWidget();
	dummy->setFixedWidth(4);
	_glayout->addWidget(dummy,row,col);
}


void DialogViewConfiguration::moveSelection(DialogViewConfigurationWidget* from, DialogViewConfigurationWidget* to)
{
  foreach ( QListWidgetItem* item, from->selectedItems() )
  {
     QListWidgetItem *clonedItem = item->clone();
    to->addItem ( clonedItem );
    to->setCurrentItem(clonedItem);
    delete item;
  }
}

void DialogViewConfiguration::moveSelectionToActiveList()
{
  moveSelection(_qlwInactive, _qlw);
}

void DialogViewConfiguration::moveSelectionToInactiveList()
{
  moveSelection(_qlw, _qlwInactive);
}

void DialogViewConfiguration::selectionChangedActive()
{
//   bool activeIsNotEmpty = _qlw->selectedItems().isEmpty();
  moveRightButton->setEnabled(! _qlw->selectedItems().isEmpty());
  moveLeftButton->setEnabled(false);
}

void DialogViewConfiguration::selectionChangedInactive()
{
  moveLeftButton->setEnabled(! _qlwInactive->selectedItems().isEmpty());
  moveRightButton->setEnabled(false);
}

/**
 * Create basic widgets of the Dialog.
 */
void DialogViewConfiguration::createPage()
{
   QList<QWidget *> &mdws = _view._mdws;

   QLabel *l1 = new QLabel( i18n("Visible channels") );
   _glayout->addWidget(l1,0,0);
      
   QLabel *l2 = new QLabel( i18n("Available channels") );
   _glayout->addWidget(l2,0,6);

   _qlwInactive = new DialogViewConfigurationWidget(frame);
   _qlwInactive->setDragDropMode(QAbstractItemView::DragDrop);
   _qlwInactive->setActiveList(false);
   _glayout->addWidget(_qlwInactive,1,6);
   connect(_qlwInactive, SIGNAL(dropped(DialogViewConfigurationWidget*,int,DialogViewConfigurationItem*,bool)),
           this  ,         SLOT(slotDropped(DialogViewConfigurationWidget*,int,DialogViewConfigurationItem*,bool)));
   
   addSpacer(1,1);
   const KIcon& icon = KIcon( QLatin1String( "arrow-left" ));
    moveLeftButton = new QPushButton(icon, "");
    moveLeftButton->setEnabled(false);
   _glayout->addWidget(moveLeftButton,1,2);
   connect(moveLeftButton, SIGNAL(clicked(bool)), SLOT(moveSelectionToActiveList()));
   addSpacer(1,3);

   const KIcon& icon2 = KIcon( QLatin1String( "arrow-right" ));
    moveRightButton = new QPushButton(icon2, "");
    moveRightButton->setEnabled(false);
   _glayout->addWidget(moveRightButton,1,4);
   connect(moveRightButton, SIGNAL(clicked(bool)), SLOT(moveSelectionToInactiveList()));
   addSpacer(1,5);

   _qlw = new DialogViewConfigurationWidget(frame);
   _glayout->addWidget(_qlw,1,0);
    connect(_qlw  ,     SIGNAL(dropped(DialogViewConfigurationWidget*,int,DialogViewConfigurationItem*,bool)),
            this  ,       SLOT(slotDropped(DialogViewConfigurationWidget*,int,DialogViewConfigurationItem*,bool)));

    // --- CONTROLS IN THE GRID ------------------------------------
   //QPalette::ColorRole bgRole;
   for ( int i=0; i<mdws.count(); ++i )
   {
       //if ( i%2 == 0) bgRole = QPalette::Base; else bgRole = QPalette::AlternateBase;
      QWidget *qw = mdws[i];
      if ( qw->inherits("MixDeviceWidget") ) {
            MixDeviceWidget *mdw = static_cast<MixDeviceWidget*>(qw);
            shared_ptr<MixDevice> md = mdw->mixDevice();
            QString mdName = md->readableName();

            int splitted = -1;
            if ( ! md->isEnum() ) {
               splitted =  ( md->playbackVolume().count() > 1) || ( md->captureVolume().count() > 1 ) ;
            }

            //qDebug()  << "add DialogViewConfigurationItem: " << mdName << " visible=" << mdw->isVisible() << "splitted=" << splitted;
            if ( mdw->isVisible() ) {
              new DialogViewConfigurationItem(_qlw, md->id(), mdw->isVisible(), mdName, splitted, mdw->mixDevice()->iconName());
            }
            else {
              new DialogViewConfigurationItem(_qlwInactive, md->id(), mdw->isVisible(), mdName, splitted, mdw->mixDevice()->iconName());
            }

/*
            if ( ! md->isEnum() && ( ( md->playbackVolume().count() > 1) || ( md->captureVolume().count() > 1) ) ) {
                cb = new QCheckBox( "", vboxForScrollView ); // split
                cb->setBackgroundRole(bgRole);
                cb->setAutoFillBackground(true);
                _qSplitCB.append(cb);
                cb->setChecked( ! mdw->isStereoLinked() );
                grid->addWidget(cb,1+i,1);
            }
            else {
                _qSplitCB.append(0);
            }
*/
            /*
            if ( ! md->isEnum() && ( md->playbackVolume().count() + md->captureVolume().count() >0 ) ) {
                cb = new QCheckBox( "", vboxForScrollView ); // limit
                cb->setBackgroundRole(bgRole);
                cb->setAutoFillBackground(true);
                _qLimitCB.append(cb);
                grid->addWidget(cb,1+i,2);
            }
            else {
            */
                //_qLimitCB.append(0);
            /*}*/
      } // is not enum
   } // for all MDW's

   
      connect(_qlwInactive, SIGNAL(itemSelectionChanged()),
           this  ,         SLOT(selectionChangedInactive()));
      connect(_qlw, SIGNAL(itemSelectionChanged()),
           this  ,         SLOT(selectionChangedActive()));

//   scrollArea->updateGeometry();
   updateGeometry();
   connect( this, SIGNAL(okClicked())   , this, SLOT(apply()) );

#ifndef QT_NO_ACCESSIBILITY
    moveLeftButton->setAccessibleName( i18n("Show the selected channel") );
    moveRightButton->setAccessibleName( i18n("Hide the selected channel") );
    _qlw->setAccessibleName( i18n("Visible channels") );
    _qlwInactive->setAccessibleName( i18n("Available channels") );
#endif
}

DialogViewConfiguration::~DialogViewConfiguration()
{
}


void DialogViewConfiguration::apply()
{
    // --- We have a 3-Step Apply of the Changes -------------------------------

    // -1- Update view and profile *****************************************
   GUIProfile* prof = _view.guiProfile();
   GUIProfile::ControlSet& oldControlset = prof->getControls();
   GUIProfile::ControlSet newControlset;

   QAbstractItemModel* model;
   model = _qlw->model();
   prepareControls(model, true, oldControlset, newControlset);
   model = _qlwInactive->model();
   prepareControls(model, false, oldControlset, newControlset);

   // -2- Copy all mandatory "catch-all" controls form the old to the new ControlSet  *******
   foreach ( ProfControl* pctl, oldControlset)
   {
       if ( pctl->isMandatory() ) {
           ProfControl* newCtl = new ProfControl(*pctl);
           newCtl->show = "full"; // The user has selected controls => mandatory controls are now only neccesary in extended or full mode
           newControlset.push_back(newCtl);
       }
   }
	
	prof->setControls(newControlset);
    prof->finalizeProfile();
    prof->setDirty();

   // --- Step 3: Tell the view, that it has changed (probably it needs some "polishing" ---
    if ( _view.getMixers().size() == 1 )
      ControlManager::instance().announce(_view.getMixers().first()->id(), ControlChangeType::ControlList, QString("View Configuration Dialog"));
    else
      ControlManager::instance().announce(QString(), ControlChangeType::ControlList, QString("View Configuration Dialog"));
}

void DialogViewConfiguration::prepareControls(QAbstractItemModel* model, bool isActiveView, GUIProfile::ControlSet& oldCtlSet, GUIProfile::ControlSet& newCtlSet)
{
    int numRows = model->rowCount();
    for (int row = 0; row < numRows; ++row) {
        // -1- Extract the value from the model ***************************
        QModelIndex index = model->index(row, 0);
        QVariant vdci;
        vdci = model->data(index, Qt::ToolTipRole);   // TooltipRole stores the ID (well, thats not really clean design, but it works)
        QString ctlId = vdci.toString();


        // -2- Find the mdw, und update it **************************
        foreach ( QWidget *qw, _view._mdws )
        {
            MixDeviceWidget *mdw = dynamic_cast<MixDeviceWidget*>(qw);
            if ( !mdw ) {
                continue;
            }

            if ( mdw->mixDevice()->id() == ctlId ) {
                mdw->setVisible(isActiveView);
                break;
            } // mdw was found
        }  // find mdw


         // -3- Insert it in the new ControlSet **************************
//         kDebug() << "Should add to new ControlSet: " << ctlId;
        foreach ( ProfControl* control, oldCtlSet)
        {
            //kDebug() << " checking " << control->id;
            QRegExp idRegexp(control->id);
            if ( ctlId.contains(idRegexp) ) {
                // found. Create a copy
                ProfControl* newCtl = new ProfControl(*control);
                newCtl->id =  '^' + ctlId + '$'; // Replace the (possible generic) regexp by the actual ID
                // We have made this an an actual control. As it is derived (from e.g. ".*") it is NOT mandatory.
                newCtl->setMandatory(false);
                if ( isActiveView ) {
                    newCtl->show = "simple";
                }
                else {
                    newCtl->show = "extended";
                }
                newCtlSet.push_back(newCtl);
//                 kDebug() << "Added to new ControlSet (done): " << newCtl->id;
                break;
            }
        }
    }

}

#include "dialogviewconfiguration.moc"

