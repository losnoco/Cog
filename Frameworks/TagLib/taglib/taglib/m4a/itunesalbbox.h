#ifndef ITUNESALBBOX_H
#define ITUNESALBBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesAlbBox: public Mp4IsoBox
    {
    public:
      ITunesAlbBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesAlbBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesAlbBoxPrivate;
      ITunesAlbBoxPrivate* d;
    }; // class ITunesAlbBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESALBBOX_H
