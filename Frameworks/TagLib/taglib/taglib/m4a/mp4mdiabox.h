#ifndef MP4MDIABOX_H
#define MP4MDIABOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4MdiaBox: public Mp4IsoBox
    {
    public:
      Mp4MdiaBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4MdiaBox();

      //! parse mdia contents
      void parse();

    private:
      class Mp4MdiaBoxPrivate;
      Mp4MdiaBoxPrivate* d;
    }; // Mp4MdiaBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4MDIABOX_H
