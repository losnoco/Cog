#ifndef MP4MOOVBOX_H
#define MP4MOOVBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4MoovBox: public Mp4IsoBox
    {
    public:
      Mp4MoovBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4MoovBox();

      //! parse moov contents
      void parse();

    private:
      class Mp4MoovBoxPrivate;
      Mp4MoovBoxPrivate* d;
    }; // Mp4MoovBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4MOOVBOX_H
