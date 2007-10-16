#ifndef ITUNESNAMBOX_H
#define ITUNESNAMBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesNamBox: public Mp4IsoBox
    {
    public:
      ITunesNamBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesNamBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesNamBoxPrivate;
      ITunesNamBoxPrivate* d;
    }; // class ITunesNamBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESNAMBOX_H
