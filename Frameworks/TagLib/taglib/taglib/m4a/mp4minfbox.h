#ifndef MP4MINFBOX_H
#define MP4MINFBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4MinfBox: public Mp4IsoBox
    {
    public:
      Mp4MinfBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4MinfBox();

      //! parse minf contents
      void parse();
      //! set the handler type - needed for stsd
      void setHandlerType( MP4::Fourcc fourcc );

    private:
      class Mp4MinfBoxPrivate;
      Mp4MinfBoxPrivate* d;
    }; // Mp4MinfBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4MINFBOX_H
