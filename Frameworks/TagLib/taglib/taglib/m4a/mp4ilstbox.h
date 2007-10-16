#ifndef MP4ILSTBOX_H
#define MP4ILSTBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4IlstBox: public Mp4IsoBox
    {
    public:
      Mp4IlstBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4IlstBox();

      //! parse ilst contents
      void parse();

    private:
      class Mp4IlstBoxPrivate;
      Mp4IlstBoxPrivate* d;
    }; // Mp4IlstBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4ILSTBOX_H
