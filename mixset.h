#ifndef MixSet_h
#define MixSet_h

#include "mixdevice.h"

class MixSet : public QPtrList<MixDevice>
{
   public:
      void read( KConfig *config, const QString& grp );
      void write( KConfig *config, const QString& grp );

      void clone( MixSet &orig );

      QString name() { return m_name; };
      void setName( const QString &name );

   private:
      QString m_name;
};

#endif
