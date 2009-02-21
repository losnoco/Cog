#ifndef MP4ISOFULLBOX_H
#define MP4ISOFULLBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4IsoFullBox : public Mp4IsoBox
    {
    public:
      //! constructor for full box
      Mp4IsoFullBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      //! destructor for mp4 iso full box
      virtual ~Mp4IsoFullBox();

      //! function to get the version of box
      uchar version();
      //! function to get the flag map
      uint  flags();
      
      //! parse wrapper to get common interface for both box and fullbox
      virtual void  parsebox();

    protected:
      class Mp4IsoFullBoxPrivate;
      Mp4IsoFullBoxPrivate* d;
    };

  } // namespace MP4
} // namespace TagLib

#endif // MP4ISOFULLBOX_H

