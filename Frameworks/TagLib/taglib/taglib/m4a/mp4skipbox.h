#ifndef MP4SKIPBOX_H
#define MP4SKIPBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4SkipBox: public Mp4IsoBox
    {
    public:
      Mp4SkipBox( File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~Mp4SkipBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class Mp4SkipBoxPrivate;
      Mp4SkipBoxPrivate* d;
    }; // class Mp4SkipBox
    
  } // namespace MP4
} // namespace TagLib

#endif // MP4SKIPBOX_H
