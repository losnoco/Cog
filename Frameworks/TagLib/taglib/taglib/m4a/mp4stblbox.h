#ifndef MP4STBLBOX_H
#define MP4STBLBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4StblBox: public Mp4IsoBox
    {
    public:
      Mp4StblBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4StblBox();

      //! parse stbl contents
      void parse();
      //! set the handler type - needed for stsd
      void setHandlerType( MP4::Fourcc fourcc );

    private:
      class Mp4StblBoxPrivate;
      Mp4StblBoxPrivate* d;
    }; // Mp4StblBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4STBLBOX_H
