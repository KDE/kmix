#ifndef KMIXTOOLBOX_H
#define KMIXTOOLBOX_H

#include "qptrlist.h"
#include "qwidget.h"

class Mixer;

class KConfig;

/**
 * This toolbox contains various static methods that are shared throughout KMix.
 * The reason, why it is not put in a common base class is, that the classes are
 * very different and cannot be changed (e.g. KPanelApplet) without major headache.
 */

class KMixToolBox {
 public:
    static void setIcons  (QPtrList<QWidget> &mdws, bool on );
    static void setLabels (QPtrList<QWidget> &mdws, bool on );
    static void setTicks  (QPtrList<QWidget> &mdws, bool on );
    static void setValueStyle  (QPtrList<QWidget> &mdws, int vs );
    static void loadConfig(QPtrList<QWidget> &mdws, KConfig *config, const QString &grp, const QString &viewPrefix  );
    static void saveConfig(QPtrList<QWidget> &mdws, KConfig *config, const QString &grp, const QString &viewPrefix  );
};
    

#endif
