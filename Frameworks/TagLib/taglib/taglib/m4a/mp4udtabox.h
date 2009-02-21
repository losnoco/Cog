#ifndef MP4UDTABOX_H
#define MP4UDTABOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4UdtaBox: public Mp4IsoBox
    {
    public:
      Mp4UdtaBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4UdtaBox();

      //! parse moov contents
      void parse();

    private:
      class Mp4UdtaBoxPrivate;
      Mp4UdtaBoxPrivate* d;
    }; // Mp4UdtaBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4UDTABOX_H
