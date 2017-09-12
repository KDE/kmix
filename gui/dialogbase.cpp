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

#include "dialogbase.h"

#include <qlayout.h>
#include <qdesktopwidget.h>
#include <qdebug.h>
#include <qstyle.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <QSpacerItem>

#include <kguiitem.h>

#include "dialogstatesaver.h"


DialogBase::DialogBase(QWidget *pnt)
    : QDialog(pnt)
{
    qDebug();

    setModal(true);					// convenience, can reset if necessary

    mMainWidget = nullptr;				// caller not provided yet
    mStateSaver = new DialogStateSaver(this);		// use our own as default

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, this);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &DialogBase::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &DialogBase::reject);
}


void DialogBase::showEvent(QShowEvent *ev)
{
    if (layout()==nullptr)					// layout not yet set up
    {
        qDebug() << "set up layout";
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);

        if (mMainWidget==nullptr)
        {
            qWarning() << "No main widget set for" << objectName();
            mMainWidget = new QWidget(this);
        }

        // Our own QVBoxLayout provides margins inside the dialogue box.
        // Therefore the main widget does not also need to have a margin.
        QLayout *contentsLayout = mMainWidget->layout();
        if (contentsLayout!=nullptr) contentsLayout->setContentsMargins(0, 0, 0, 0);

        mainLayout->addWidget(mMainWidget);
        mainLayout->setStretchFactor(mMainWidget, 1);

        mainLayout->addWidget(mButtonBox);
    }

    QDialog::showEvent(ev);				// show the dialogue
}


void DialogBase::setButtons(QDialogButtonBox::StandardButtons buttons)
{
    qDebug() << buttons;
    mButtonBox->setStandardButtons(buttons);

    if (buttons & QDialogButtonBox::Ok)
    {
        qDebug() << "set up OK button";
        QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL|Qt::Key_Return);
    }

// TODO: set F1 shortcut for Help?

}


void DialogBase::setButtonEnabled(QDialogButtonBox::StandardButton button, bool state)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=nullptr) but->setEnabled(state);
}


void DialogBase::setButtonText(QDialogButtonBox::StandardButton button, const QString &text)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=nullptr) but->setText(text);
}

void DialogBase::setButtonIcon(QDialogButtonBox::StandardButton button, const QIcon &icon)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=nullptr) but->setIcon(icon);
}

void DialogBase::setButtonGuiItem(QDialogButtonBox::StandardButton button, const KGuiItem &guiItem)
{
    QPushButton *but = mButtonBox->button(button);
    if (but!=nullptr) KGuiItem::assign(but, guiItem);
}


int DialogBase::spacingHint()
{
    // from KDE4 KDialog::spacingHint()
    return (QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing));
}


int DialogBase::verticalSpacing()
{
    int spacing = QApplication::style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    if (spacing==-1) spacing = QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
    return (spacing);
}


int DialogBase::horizontalSpacing()
{
    int spacing = QApplication::style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    if (spacing==-1) spacing = QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
    return (spacing);
}


QSpacerItem *DialogBase::verticalSpacerItem()
{
    return (new QSpacerItem(1, verticalSpacing(), QSizePolicy::Minimum, QSizePolicy::Fixed));
}


QSpacerItem *DialogBase::horizontalSpacerItem()
{
    return (new QSpacerItem(horizontalSpacing(), 1, QSizePolicy::Fixed, QSizePolicy::Minimum));
}


void DialogBase::setStateSaver(DialogStateSaver *saver)
{
    if (mStateSaver!=nullptr) delete mStateSaver;
    mStateSaver = saver;
}
