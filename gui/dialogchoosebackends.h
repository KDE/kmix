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
#ifndef DIALOGCHOOSEBACKENDS_H
#define DIALOGCHOOSEBACKENDS_H

#include <qwidget.h>

class QListWidget;
class QListWidgetItem;

class Mixer;


class DialogChooseBackends : public QWidget
{
	Q_OBJECT

public:
	DialogChooseBackends(QWidget* parent, const QSet<QString>& backends);
	virtual ~DialogChooseBackends() = default;

	QSet<QString> getChosenBackends();
	bool getAndResetModifyFlag();
	bool getModifyFlag() const;

signals:
	void backendsModified();

private:
	void createWidgets(const QSet<QString>& backends);
	void createPage(const QSet<QString>& backends);

	QListWidget *m_mixerList;
	bool modified;

private slots:
	void backendsModifiedSlot();
	void itemActivatedSlot(QListWidgetItem *item);
};

#endif
