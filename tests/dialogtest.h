#ifndef DIALOGTEST_H
#define DIALOGTEST_H

#include <QCheckBox>

#include <KApplication>


class App : public KApplication
{
    Q_OBJECT
    public:
        App();
};

class DialogTest : public QDialog
{
    Q_OBJECT
    public:
        DialogTest();
};

#endif
