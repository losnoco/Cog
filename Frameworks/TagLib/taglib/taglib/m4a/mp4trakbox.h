#ifndef MP4TRAKBOX_H
#define MP4TRAKBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4TrakBox: public Mp4IsoBox
    {
    public:
      Mp4TrakBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4TrakBox();

      //! parse trak contents
      void parse();

    private:
      class Mp4TrakBoxPrivate;
      Mp4TrakBoxPrivate* d;
    }; // Mp4TrakBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4TRAKBOX_H
