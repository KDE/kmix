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

#include <QCheckBox>
#include <qlist.h>
class QVBoxLayout;

#include <kdialog.h>

#include "viewbase.h"


class DialogViewConfiguration : public KDialog
{
    Q_OBJECT
 public:
    DialogViewConfiguration(QWidget* parent, ViewBase& view);
    ~DialogViewConfiguration();

    QSize sizeHint() const;
 public slots:
    void apply();

 private:
    QVBoxLayout* _layout;
    ViewBase&    _view;
    QList<QCheckBox *>  _qEnabledCB;
};

#endif
