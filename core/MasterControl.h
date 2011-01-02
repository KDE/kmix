/*
 * MasterControl.h
 *
 *  Created on: 02.01.2011
 *      Author: kde
 */

#ifndef MASTERCONTROL_H_
#define MASTERCONTROL_H_

#include <QString>

class MasterControl
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
