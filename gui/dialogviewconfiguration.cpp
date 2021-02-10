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

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>

#include <klocalizedstring.h>

#include "gui/guiprofile.h"
#include "gui/mixdevicewidget.h"
#include "core/ControlManager.h"
#include "core/mixdevice.h"
#include "core/mixer.h"

/**
 * Standard constructor.
 */
DialogViewConfigurationItem::DialogViewConfigurationItem(QListWidget *parent, const QString &id, bool shown, const QString &name, int splitted, const QString& iconName) :
   QListWidgetItem(parent), _id(id), _shown(shown), _name(name), _splitted(splitted), _iconName(iconName)
{
  refreshItem();
}

/**
 * Deserializing constructor.  Used for DnD.
 */
DialogViewConfigurationItem::DialogViewConfigurationItem(QListWidget *parent, QDataStream &s)
    : QListWidgetItem(parent)
{
  s >> _id;
  s >> _shown;
  s >> _name;
  s >> _splitted;
  s >> _iconName;

  refreshItem();
}

void DialogViewConfigurationItem::refreshItem()
{
  setFlags((flags() | Qt::ItemIsDragEnabled) &  ~Qt::ItemIsDropEnabled);
  setText(_name);
  setIcon(QIcon::fromTheme(_iconName));
  setData(Qt::ToolTipRole, _id);  // a hack. I am giving up to do it right
  setData(Qt::DisplayRole, _name);
}


/**
 * Serializer.  Used for DnD.
 */
QDataStream &DialogViewConfigurationItem::serialize(QDataStream &s) const
{
    s << _id;
    s << _shown;
    s << _name;
    s << _splitted;
    s << _iconName;
    return (s);
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

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    // we only support single selection
    DialogViewConfigurationItem* item = static_cast<DialogViewConfigurationItem *>(items.first());
    stream << *item;

    bool active = isActiveList();
    mimedata->setData("application/x-kde-action-list", data);
    mimedata->setData("application/x-kde-source-treewidget", active ? "active" : "inactive");

    return mimedata;
}

bool DialogViewConfigurationWidget::dropMimeData(int index, const QMimeData * mimeData, Qt::DropAction /*action*/)
{
    const QByteArray data = mimeData->data("application/x-kde-action-list");
    if (data.isEmpty()) return false;

    QDataStream stream(data);
    DialogViewConfigurationItem* item = new DialogViewConfigurationItem(nullptr, stream);
    emit dropped(this, index, item);
    return true;
}

DialogViewConfiguration::DialogViewConfiguration(QWidget *parent, ViewBase &view)
    : DialogBase(parent),
      _view(view)
{
   setWindowTitle( i18n( "Configure Channels" ) );
   setButtons( QDialogButtonBox::Ok|QDialogButtonBox::Cancel );

   QWidget *frame = new QWidget( this );
   frame->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
   setMainWidget( frame );
   
   // The _layout will hold two items: The title and the Drag-n-Drop area
   QVBoxLayout *layout = new QVBoxLayout(frame);
   
   // --- HEADER ---
   QLabel *qlb = new QLabel( i18n("Configure the visible channels. Drag icons between the lists to update."), frame );
   layout->addWidget(qlb);
   
   _glayout = new QGridLayout();
   _glayout->setContentsMargins(0, 0, 0, 0);
   layout->addLayout(_glayout);

   createPage();
}



/**
 * Drop an item from one list to the other.
 */
void DialogViewConfiguration::slotDropped(DialogViewConfigurationWidget *list, int index, DialogViewConfigurationItem *item)
{
    //qCDebug(KMIX_LOG) << "dropped item (index" << index << "): " << item->_id << item->_shown << item->_name << item->_splitted << item->_iconName;
    list->insertItem(index, item);
}


void DialogViewConfiguration::addSpacer(int row, int col)
{
	QWidget *dummy = new QWidget();
	dummy->setFixedWidth(4);
	_glayout->addWidget(dummy,row,col);
}


void DialogViewConfiguration::moveSelection(DialogViewConfigurationWidget *from, DialogViewConfigurationWidget *to)
{
    const QList<QListWidgetItem *> sel = from->selectedItems();
    from->selectionModel()->clearSelection();

    for (QListWidgetItem *item : qAsConst(sel))
    {
        from->takeItem(from->row(item));
        to->addItem(item);
        to->setCurrentItem(item);
    }
}

void DialogViewConfiguration::moveSelectionToActiveList()
{
  moveSelection(_qlwInactive, _qlwActive);
}

void DialogViewConfiguration::moveSelectionToInactiveList()
{
  moveSelection(_qlwActive, _qlwInactive);
}

void DialogViewConfiguration::selectionChangedActive()
{
  moveRightButton->setEnabled(!_qlwActive->selectedItems().isEmpty());
  moveLeftButton->setEnabled(false);
}

void DialogViewConfiguration::selectionChangedInactive()
{
  moveLeftButton->setEnabled(!_qlwInactive->selectedItems().isEmpty());
  moveRightButton->setEnabled(false);
}

/**
 * Create basic widgets of the Dialog.
 */
void DialogViewConfiguration::createPage()
{
   QLabel *l1 = new QLabel( i18n("Visible channels:") );
   _glayout->addWidget(l1,0,0);
      
   QLabel *l2 = new QLabel( i18n("Available channels:") );
   _glayout->addWidget(l2,0,6);

   QWidget *frame = mainWidget();
   _qlwInactive = new DialogViewConfigurationWidget(frame);
   _qlwInactive->setDragDropMode(QAbstractItemView::DragDrop);
   _qlwInactive->setActiveList(false);
   _glayout->addWidget(_qlwInactive,1,6);
   connect(_qlwInactive, &DialogViewConfigurationWidget::dropped, this, &DialogViewConfiguration::slotDropped);
   
   addSpacer(1,1);
   const QIcon& icon = QIcon::fromTheme( QLatin1String( "arrow-left" ));
    moveLeftButton = new QPushButton(icon, "");
    moveLeftButton->setEnabled(false);
    moveLeftButton->setToolTip(i18n("Move the selected channel to the visible list"));
   _glayout->addWidget(moveLeftButton,1,2);
   connect(moveLeftButton, &QPushButton::clicked, this, &DialogViewConfiguration::moveSelectionToActiveList);
   addSpacer(1,3);

   const QIcon& icon2 = QIcon::fromTheme( QLatin1String( "arrow-right" ));
    moveRightButton = new QPushButton(icon2, "");
    moveRightButton->setEnabled(false);
    moveRightButton->setToolTip(i18n("Move the selected channel to the available (hidden) list"));
   _glayout->addWidget(moveRightButton,1,4);
   connect(moveRightButton, &QPushButton::clicked, this, &DialogViewConfiguration::moveSelectionToInactiveList);
   addSpacer(1,5);

   _qlwActive = new DialogViewConfigurationWidget(frame);
   _glayout->addWidget(_qlwActive,1,0);
   connect(_qlwActive, &DialogViewConfigurationWidget::dropped, this, &DialogViewConfiguration::slotDropped);

    // --- CONTROLS IN THE GRID ------------------------------------
   //QPalette::ColorRole bgRole;

    const int num = _view.mixDeviceCount();
    for (int i = 0; i<num; ++i)
    {
        const MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(_view.mixDeviceAt(i));
        if (mdw==nullptr) continue;

       //if ( i%2 == 0) bgRole = QPalette::Base; else bgRole = QPalette::AlternateBase;
            const shared_ptr<MixDevice> md = mdw->mixDevice();
            const QString mdName = md->readableName();

            int splitted = -1;
            if ( ! md->isEnum() ) {
               splitted =  ( md->playbackVolume().count() > 1) || ( md->captureVolume().count() > 1 ) ;
            }

            //qCDebug(KMIX_LOG)  << "add DialogViewConfigurationItem: " << mdName << " visible=" << mdw->isVisible() << "splitted=" << splitted;
            auto *item = new DialogViewConfigurationItem(nullptr, md->id(), true, mdName, splitted, mdw->mixDevice()->iconName());
            if (mdw->isVisible()) _qlwActive->addItem(item);
            else _qlwInactive->addItem(item);
    } // for all MDW's

    connect(_qlwInactive, &QListWidget::itemSelectionChanged, this, &DialogViewConfiguration::selectionChangedInactive);
    connect(_qlwActive, &QListWidget::itemSelectionChanged, this, &DialogViewConfiguration::selectionChangedActive);

    updateGeometry();
    connect(this, &QDialog::accepted, this, &DialogViewConfiguration::apply);

#ifndef QT_NO_ACCESSIBILITY
    moveLeftButton->setAccessibleName( i18n("Show the selected channel") );
    moveRightButton->setAccessibleName( i18n("Hide the selected channel") );
    _qlwActive->setAccessibleName( i18n("Visible channels") );
    _qlwInactive->setAccessibleName( i18n("Available channels") );
#endif
}


void DialogViewConfiguration::apply()
{
    // --- We have a 3-Step Apply of the Changes -------------------------------

    // -1- Update view and profile *****************************************
   GUIProfile* prof = _view.guiProfile();
   const GUIProfile::ControlSet &oldControlset = prof->getControls();
   GUIProfile::ControlSet newControlset;

   QAbstractItemModel* model;
   model = _qlwActive->model();
   prepareControls(model, true, oldControlset, newControlset);
   model = _qlwInactive->model();
   prepareControls(model, false, oldControlset, newControlset);

   // -2- Copy all mandatory "catch-all" controls form the old to the new ControlSet  *******
   for (const ProfControl *pctl : qAsConst(oldControlset))
   {
       if ( pctl->isMandatory() ) {
           ProfControl* newCtl = new ProfControl(*pctl);
           // The user has selected controls => mandatory controls (RegExp templates) should not been shown any longer
           newCtl->setVisibility(GuiVisibility::Never);
           newControlset.push_back(newCtl);
       }
   }
	
	prof->setControls(newControlset);
    prof->setDirty();

   // --- Step 3: Tell the view, that it has changed (probably it needs some "polishing" ---
    if ( _view.getMixers().size() == 1 )
      ControlManager::instance().announce(_view.getMixers().first()->id(), ControlManager::ControlList, QString("View Configuration Dialog"));
    else
      ControlManager::instance().announce(QString(), ControlManager::ControlList, QString("View Configuration Dialog"));
}

void DialogViewConfiguration::prepareControls(QAbstractItemModel* model, bool isActiveView, const GUIProfile::ControlSet &oldCtlSet, GUIProfile::ControlSet &newCtlSet)
{
    const int numRows = model->rowCount();
    const int num = _view.mixDeviceCount();

    for (int row = 0; row < numRows; ++row) {
        // -1- Extract the value from the model ***************************
        QModelIndex index = model->index(row, 0);
        QVariant vdci;
        vdci = model->data(index, Qt::ToolTipRole);   // TooltipRole stores the ID (well, thats not really clean design, but it works)
        QString ctlId = vdci.toString();

        // -2- Find the mdw, und update it **************************
        for (int i = 0; i<num; ++i)
        {
            MixDeviceWidget *mdw = qobject_cast<MixDeviceWidget *>(_view.mixDeviceAt(i));
            if (mdw==nullptr) continue;

            if ( mdw->mixDevice()->id() == ctlId ) {
                mdw->setVisible(isActiveView);
                break;
            } // mdw was found
        }  // find mdw


         // -3- Insert it in the new ControlSet **************************
//         qCDebug(KMIX_LOG) << "Should add to new ControlSet: " << ctlId;
        for (const ProfControl *control : qAsConst(oldCtlSet))
        {
            //qCDebug(KMIX_LOG) << " checking " << control->id;
            QRegExp idRegexp(control->id());
            if ( ctlId.contains(idRegexp) ) {
                // found. Create a copy
                ProfControl* newCtl = new ProfControl(*control);
                newCtl->setId('^' + ctlId + '$'); // Replace the (possible generic) regexp by the actual ID
                // We have made this an an actual control. As it is derived (from e.g. ".*") it is NOT mandatory.
                newCtl->setMandatory(false);
                newCtl->setVisible(isActiveView);
                newCtlSet.push_back(newCtl);
//                 qCDebug(KMIX_LOG) << "Added to new ControlSet (done): " << newCtl->id;
                break;
            }
        }
    }

}


