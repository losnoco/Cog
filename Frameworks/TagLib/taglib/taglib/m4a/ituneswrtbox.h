#ifndef ITUNESWRTBOX_H
#define ITUNESWRTBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesWrtBox: public Mp4IsoBox
    {
    public:
      ITunesWrtBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesWrtBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesWrtBoxPrivate;
      ITunesWrtBoxPrivate* d;
    }; // class ITunesWrtBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESWRTBOX_H
