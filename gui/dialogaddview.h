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
#ifndef DIALOGADDVIEW_H
#define DIALOGADDVIEW_H

class QButtonGroup;
#include <QStringList>
class KComboBox;
#include <qradiobutton.h>
class QScrollArea;
#include <kvbox.h>
class QVBoxLayout;

#include <kdialog.h>

class Mixer;

class DialogAddView : public KDialog
{
    Q_OBJECT
 public:
    DialogAddView(QWidget* parent, Mixer*);
    ~DialogAddView();

    QString getresultViewName() { return resultViewName; }
    QString getresultMixerId() { return resultMixerId; }

 public slots:
    void apply();

 private:
    void createWidgets(Mixer*);
    void createPage(Mixer*);
    QVBoxLayout* _layout;
    KComboBox* m_cMixer;
    QScrollArea* m_scrollableChannelSelector;
    KVBox *m_vboxForScrollView;
    QButtonGroup *m_buttonGroupForScrollView;
    QFrame *m_mainFrame;
    static QStringList viewNames;
    static QStringList viewIds;

    QString resultViewName;
    QString resultMixerId;

 private slots:
   void createPageByID(int mixerId);
   void profileRbtoggled(bool selected);
};

#endif
