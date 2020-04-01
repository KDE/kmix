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
#include <qscreen.h>
#include <qwindow.h>

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


void DialogStateSaver::restoreWindowState(QWidget *widget)
{
    const KConfigGroup grp = configGroupFor(widget);
    restoreWindowState(widget, grp);
}


void DialogStateSaver::restoreWindowState(QWidget *widget, const KConfigGroup &grp)
{
    // Ensure that the widget's window() - that is, either the widget itself
    // or its nearest ancestor widget that is or could be top level - is a
    // native window, so that windowHandle() below will return a valid QWindow.
    const WId wid = widget->window()->winId();
    const QRect desk = widget->window()->windowHandle()->screen()->geometry();
    const QSize sizeDefault = widget->sizeHint();

    // originally from KDE4 KDialog::restoreDialogSize()
    qDebug() << "from" << grp.name() << "in" << grp.config()->name();
    const int width = grp.readEntry(QString::fromLatin1("Width %1").arg(desk.width()), sizeDefault.width());
    const int height = grp.readEntry(QString::fromLatin1("Height %1").arg(desk.height()), sizeDefault.height());
    widget->resize(width, height);
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


void DialogStateSaver::saveWindowState(QWidget *widget)
{
    KConfigGroup grp = configGroupFor(widget);
    saveWindowState(widget, grp);
}


void DialogStateSaver::saveWindowState(QWidget *widget, KConfigGroup &grp)
{
    const WId wid = widget->window()->winId();
    const QRect desk = widget->window()->windowHandle()->screen()->geometry();
    const QSize sizeToSave = widget->size();

    // originally from KDE4 KDialog::saveDialogSize()
    qDebug() << "to" << grp.name() << "in" << grp.config()->name();
    grp.writeEntry(QString::fromLatin1("Width %1").arg(desk.width()), sizeToSave.width());
    grp.writeEntry( QString::fromLatin1("Height %1").arg(desk.height()), sizeToSave.height());
    grp.sync();
}


void DialogStateSaver::setSaveSettingsDefault(bool on)
{
    sSaveSettings = on;
}
