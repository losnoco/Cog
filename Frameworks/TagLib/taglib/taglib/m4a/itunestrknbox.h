#ifndef ITUNESTRKNBOX_H
#define ITUNESTRKNBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesTrknBox: public Mp4IsoBox
    {
    public:
      ITunesTrknBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesTrknBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesTrknBoxPrivate;
      ITunesTrknBoxPrivate* d;
    }; // class ITunesTrknBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESTRKNBOX_H
