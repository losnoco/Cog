#ifndef BOXFACTORY_H
#define BOXFACTORY_H

#include "taglib.h"
#include "mp4isobox.h"

namespace TagLib
{
  namespace MP4
  {
    class BoxFactory
    {
    public:
      BoxFactory();
      ~BoxFactory();

      //! factory function
      Mp4IsoBox* createInstance( TagLib::File* anyfile, MP4::Fourcc fourcc, uint size, long offset ) const;
    }; // class BoxFactory

  } // namepace MP4
} // namepace TagLib

#endif // BOXFACTORY_H
