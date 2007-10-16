#ifndef ITUNESGRPBOX_H
#define ITUNESGRPBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesGrpBox: public Mp4IsoBox
    {
    public:
      ITunesGrpBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesGrpBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesGrpBoxPrivate;
      ITunesGrpBoxPrivate* d;
    }; // class ITunesGrpBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESGRPBOX_H
