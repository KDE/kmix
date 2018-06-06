/*
 * MasterControl.h
 *
 *  Created on: 02.01.2011
 *      Author: kde
 */

#ifndef MASTERCONTROL_H_
#define MASTERCONTROL_H_

#include "config.h"
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
