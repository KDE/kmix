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
    return card;
}

QString MasterControl::getControl() const
{
    return control;
}

void MasterControl::set(QString card, QString control)
{
    this->card = card;
    this->control = control;
}

bool MasterControl::isValid()
{
    if ( control.isEmpty() || card.isEmpty() )
        return false;

    return true;
}

