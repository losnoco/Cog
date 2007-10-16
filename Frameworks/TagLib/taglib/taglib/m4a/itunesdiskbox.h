#ifndef ITUNESDISKBOX_H
#define ITUNESDISKBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesDiskBox: public Mp4IsoBox
    {
    public:
      ITunesDiskBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesDiskBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesDiskBoxPrivate;
      ITunesDiskBoxPrivate* d;
    }; // class ITunesDiskBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESDISKBOX_H
