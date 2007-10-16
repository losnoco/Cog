#ifndef ITUNESCMTBOX_H
#define ITUNESCMTBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesCmtBox: public Mp4IsoBox
    {
    public:
      ITunesCmtBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesCmtBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesCmtBoxPrivate;
      ITunesCmtBoxPrivate* d;
    }; // class ITunesCmtBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESCMTBOX_H
