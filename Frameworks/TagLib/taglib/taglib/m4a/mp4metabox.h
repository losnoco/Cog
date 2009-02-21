#ifndef MP4METABOX_H
#define MP4METABOX_H

#include "mp4isofullbox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4MetaBox: public Mp4IsoFullBox
    {
    public:
      Mp4MetaBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4MetaBox();

      //! parse meta contents
      void parse();

    private:
      class Mp4MetaBoxPrivate;
      Mp4MetaBoxPrivate* d;
    }; // Mp4MetaBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4METABOX_H
