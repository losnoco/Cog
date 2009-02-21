#ifndef ITUNESCVRBOX_H
#define ITUNESCVRBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesCvrBox: public Mp4IsoBox
    {
    public:
      ITunesCvrBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesCvrBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesCvrBoxPrivate;
      ITunesCvrBoxPrivate* d;
    }; // class ITunesCvrBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESCVRBOX_H
