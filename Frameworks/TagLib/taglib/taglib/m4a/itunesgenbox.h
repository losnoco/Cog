#ifndef ITUNESGENBOX_H
#define ITUNESGENBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesGenBox: public Mp4IsoBox
    {
    public:
      ITunesGenBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesGenBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesGenBoxPrivate;
      ITunesGenBoxPrivate* d;
    }; // class ITunesGenBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESGENBOX_H
