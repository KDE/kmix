/*
 * MasterControl.cpp
 *
 *  Created on: 02.01.2011
 *      Author: kde
 */

#include "MasterControl.h"

MasterControl::MasterControl()
{
}

MasterControl::~MasterControl()
{
}

QString MasterControl::getCard() const
{
    return m_card;
}

QString MasterControl::getControl() const
{
    return m_control;
}

void MasterControl::set(QString card, QString control)
{
    this->m_card = card;
    this->m_control = control;
}

bool MasterControl::isValid()
{
    if (m_control.isEmpty() || m_card.isEmpty())
        return false;

    return true;
}

