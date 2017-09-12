/*
 * KMix -- KDE's full featured mini mixer
 *
 * Copyright (C) 2017 Jonathan Marten <jjm@keelhaul.me.uk>
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
 * License along with this program; if not, see
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "dialogstatesaver.h"

#include <qdialog.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qapplication.h>
#include <qabstractbutton.h>
#include <qdebug.h>

#include <kconfiggroup.h>
#include <ksharedconfig.h>


static bool sSaveSettings = true;


DialogStateSaver::DialogStateSaver(QDialog *pnt)
    : QObject(pnt)
{
    Q_ASSERT(pnt!=nullptr);
    mParent = pnt;
    mParent->installEventFilter(this);
    connect(mParent, &QDialog::accepted, this, &DialogStateSaver::saveConfigInternal);
}


void DialogStateSaver::setSaveOnButton(QAbstractButton *but)
{
    qDebug() << "button" << but->text();
    connect(but, &QAbstractButton::clicked, this, &DialogStateSaver::saveConfigInternal);
}


bool DialogStateSaver::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj==mParent && ev->type()==QEvent::Show)	// only interested in show event
    {
        restoreConfigInternal();			// restore size and config
    }
    return (false);					// always pass the event on
}


static KConfigGroup configGroupFor(QWidget *window)
{
    QString objName = window->objectName();
    if (objName.isEmpty())
    {
        objName = window->metaObject()->className();
        qWarning() << "object name not set, using class name" << objName;
    }
    else qDebug() << "for" << objName << "which is a" << window->metaObject()->className();

    return (KSharedConfig::openConfig(QString(), KConfig::NoCascade)->group(objName));
}


void DialogStateSaver::restoreConfigInternal()
{
    if (!sSaveSettings) return;

    const KConfigGroup grp = configGroupFor(mParent);
    this->restoreConfig(mParent, grp);
}


void DialogStateSaver::restoreConfig(QDialog *dialog, const KConfigGroup &grp)
{
    restoreWindowState(dialog, grp);
}


void DialogStateSaver::restoreWindowState(QWidget *window)
{
    const KConfigGroup grp = configGroupFor(window);
    restoreWindowState(window, grp);
}


void DialogStateSaver::restoreWindowState(QWidget *window, const KConfigGroup &grp)
{
    // from KDE4 KDialog::restoreDialogSize()
    int scnum = QApplication::desktop()->screenNumber(window->parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);
    int width = window->sizeHint().width();
    int height = window->sizeHint().height();

    qDebug() << "from" << grp.name() << "in" << grp.config()->name();
    width = grp.readEntry(QString::fromLatin1("Width %1").arg(desk.width()), width);
    height = grp.readEntry(QString::fromLatin1("Height %1").arg(desk.height()), height);
    window->resize(width, height);
}


void DialogStateSaver::saveConfigInternal() const
{
    if (!sSaveSettings) return;

    KConfigGroup grp = configGroupFor(mParent);
    this->saveConfig(mParent, grp);
    grp.sync();
}


void DialogStateSaver::saveConfig(QDialog *dialog, KConfigGroup &grp) const
{
    saveWindowState(dialog, grp);
}


void DialogStateSaver::saveWindowState(QWidget *window)
{
    KConfigGroup grp = configGroupFor(window);
    saveWindowState(window, grp);
}


void DialogStateSaver::saveWindowState(QWidget *window, KConfigGroup &grp)
{
    // from KDE4 KDialog::saveDialogSize()
    const int scnum = QApplication::desktop()->screenNumber(window->parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);
    const QSize sizeToSave = window->size();

    qDebug() << "to" << grp.name() << "in" << grp.config()->name();
    grp.writeEntry(QString::fromLatin1("Width %1").arg(desk.width()), sizeToSave.width());
    grp.writeEntry( QString::fromLatin1("Height %1").arg(desk.height()), sizeToSave.height());
    grp.sync();
}


void DialogStateSaver::setSaveSettingsDefault(bool on)
{
    sSaveSettings = on;
}
