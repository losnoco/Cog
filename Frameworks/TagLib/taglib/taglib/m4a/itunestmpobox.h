#ifndef ITUNESTMPOBOX_H
#define ITUNESTMPOBOX_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesTmpoBox: public Mp4IsoBox
    {
    public:
      ITunesTmpoBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesTmpoBox();

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesTmpoBoxPrivate;
      ITunesTmpoBoxPrivate* d;
    }; // class ITunesTmpoBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESTMPOBOX_H
