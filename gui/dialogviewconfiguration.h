//-*-C++-*-
/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright Christian Esken <esken@kde.org>
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
#ifndef DIALOGVIEWCONFIGURATION_H
#define DIALOGVIEWCONFIGURATION_H

// QT
#include <QCheckBox>
class QLabel;
#include <QGridLayout>
class QHBoxLayout;
#include <qlist.h>
#include <QListWidget>
class QScrollArea;
class QVBoxLayout;

// QT DND
#include <QDragEnterEvent>
#include <QMimeData>

// KDE
#include <kdebug.h>
#include <kdialog.h>

// KMix
#include "gui/guiprofile.h"
#include "viewbase.h"

class DialogViewConfigurationItem : public QListWidgetItem
{
 friend class QDataStream;


 public:
    DialogViewConfigurationItem( QListWidget *parent);
    DialogViewConfigurationItem( QListWidget *parent, QString id, bool shown, QString name, int splitted, const QString& iconName );

    void refreshItem();
 public:
    QString _id;
    bool _shown;
    QString _name;
    int _splitted;
    QString _iconName;
};

class DialogViewConfigurationWidget : public QListWidget
{
    Q_OBJECT
public:
    DialogViewConfigurationWidget(QWidget *parent=0);

    void setActiveList(bool isActiveList) {
        m_activeList = isActiveList;
    }
    bool isActiveList() const { return m_activeList; };

 Q_SIGNALS:
   void dropped(DialogViewConfigurationWidget* list, int index, DialogViewConfigurationItem* item, bool sourceIsActiveList);

protected:
    virtual QMimeData* mimeData(const QList<QListWidgetItem*> items) const;
    virtual bool dropMimeData(int index, const QMimeData * mimeData, Qt::DropAction action);

    virtual Qt::DropActions supportedDropActions() const
    {
        //kDebug() << "supportedDropActions!";
        return Qt::MoveAction;
    }
    virtual QStringList mimeTypes() const
    {
        //kDebug() << "mimeTypes!";
        return QStringList() << "application/x-kde-action-list";
    }

    // Skip internal dnd handling in QListWidget ---- how is one supposed to figure this out
    // without reading the QListWidget code !?
    virtual void dropEvent(QDropEvent* ev) {
        QAbstractItemView::dropEvent(ev);
    }

private:
    bool m_activeList;
};


class DialogViewConfiguration : public KDialog
{
    Q_OBJECT
 public:
    DialogViewConfiguration(QWidget* parent, ViewBase& view);
    ~DialogViewConfiguration();


  //  QSize sizeHint() const;
 public slots:
    void apply();
 
 private slots:
   void slotDropped(DialogViewConfigurationWidget* list, int index, DialogViewConfigurationItem* item, bool sourceIsActiveList );

   void moveSelectionToActiveList();
   void moveSelectionToInactiveList();
   void selectionChangedActive();
   void selectionChangedInactive();
   
 private:
    //void dragEnterEvent(QDragEnterEvent *event);
    void prepareControls(QAbstractItemModel* model, bool isActiveView, GUIProfile::ControlSet& oldCtlSet, GUIProfile::ControlSet& newCtlSet);
    void createPage();
    void addSpacer(int row, int col);
    void moveSelection(DialogViewConfigurationWidget* from, DialogViewConfigurationWidget* to);
    QVBoxLayout* _layout;
    ViewBase&    _view;
    QWidget * frame;
    QGridLayout *_glayout;

    QLabel* qlb;
    QPushButton* moveLeftButton;
    QPushButton* moveRightButton;

    DialogViewConfigurationWidget *_qlw;
    DialogViewConfigurationWidget *_qlwInactive;
};



#endif
