#ifndef MP4STSDBOX_H
#define MP4STSDBOX_H

#include "mp4isofullbox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4StsdBox: public Mp4IsoFullBox
    {
    public:
      Mp4StsdBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4StsdBox();

      //! parse stsd contents
      void parse();
      //! set the handler type - needed for stsd
      void setHandlerType( MP4::Fourcc fourcc );

    private:
      class Mp4StsdBoxPrivate;
      Mp4StsdBoxPrivate* d;
    }; // Mp4StsdBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4STSDBOX_H
