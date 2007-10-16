#ifndef ITUNESARTBOX_H
#define ITUNESARTBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesArtBox: public Mp4IsoBox
    {
    public:
      ITunesArtBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesArtBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesArtBoxPrivate;
      ITunesArtBoxPrivate* d;
    }; // class ITunesArtBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESARTBOX_H
