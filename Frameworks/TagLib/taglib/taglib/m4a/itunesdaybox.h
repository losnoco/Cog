#ifndef ITUNESDAYBOX_H
#define ITUNESDAYBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesDayBox: public Mp4IsoBox
    {
    public:
      ITunesDayBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesDayBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesDayBoxPrivate;
      ITunesDayBoxPrivate* d;
    }; // class ITunesDayBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESDAYBOX_H
