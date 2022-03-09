/*
    SPDX-FileCopyrightText: 2022 Heiko Becker <heiko.becker@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ProfileTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testReadProfile();
};

