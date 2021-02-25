/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright 2011 Christian Esken <esken@kde.org>
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

#ifndef MASTERCONTROL_H_
#define MASTERCONTROL_H_

#include "kmixcore_export.h"

#if defined(HAVE_STD_SHARED_PTR)
#include <memory>
using std::shared_ptr;
#elif defined(HAVE_STD_TR1_SHARED_PTR)
#include <tr1/memory>
using std::tr1::shared_ptr;
#endif

#include <QString>

class KMIXCORE_EXPORT MasterControl
{
public:
    MasterControl();
    virtual ~MasterControl();
    QString getCard() const;
    QString getControl() const;
    void set(QString card, QString control);

    bool isValid();

private:
    QString card;
    QString control;

};

#endif /* MASTERCONTROL_H_ */
