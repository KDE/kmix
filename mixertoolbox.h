#ifndef MIXERTOOLBOX_H
#define MIXERTOOLBOX_H

#include <qptrlist.h>
#include <qstring.h>

class Mixer;

/**
 * This toolbox contains various static methods that are shared throughout KMix.
 * It only contains no-GUI code. The shared with-GUI code is in KMixToolBox
 * The reason, why it is not put in a common base class is, that the classes are
 * very different and cannot be changed (e.g. KPanelApplet) without major headache.
 */
class MixerToolBox {
 public:
    static void initMixer(QPtrList<Mixer>&, bool, QString&);
    static void deinitMixer();
};
    

#endif
