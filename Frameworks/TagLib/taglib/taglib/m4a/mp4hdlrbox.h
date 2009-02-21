#ifndef MP4HDLRBOX_H
#define MP4HDLRBOX_H

#include "mp4isofullbox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4HdlrBox: public Mp4IsoFullBox
    {
    public:
      Mp4HdlrBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4HdlrBox();

      //! parse hdlr contents
      void parse();
      //! get the handler type
      MP4::Fourcc hdlr_type() const;
      //! get the hdlr string
      TagLib::String hdlr_string() const;

    private:
      class Mp4HdlrBoxPrivate;
      Mp4HdlrBoxPrivate* d;
    }; // Mp4HdlrBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4HDLRBOX_H
