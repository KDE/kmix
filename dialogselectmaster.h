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
 * Software Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef DIALOGSELECTMASTER_H
#define DIALOGSELECTMASTER_H

class Q3ButtonGroup;
class KComboBox;
#include <qradiobutton.h>
class Q3ScrollView;
#include <qstringlist.h>
class Q3VBox;
class QVBoxLayout;

#include <kdialogbase.h>

class Mixer;

class DialogSelectMaster : public KDialogBase
{
    Q_OBJECT
 public:
    DialogSelectMaster(Mixer *);
    ~DialogSelectMaster();

 signals:
    void newMasterSelected(int, int);

 public slots:
    void apply();

 private:
    void createWidgets(Mixer*);
    void createPage(Mixer*);
    QVBoxLayout* _layout;
    KComboBox* m_cMixer;
    Q3ScrollView* m_scrollableChannelSelector;
    Q3VBox *m_vboxForScrollView;
    Q3ButtonGroup *m_buttonGroupForScrollView;
    QStringList m_mixerPKs;

 private slots:
   void createPageByID(int mixerId);
};

#endif
