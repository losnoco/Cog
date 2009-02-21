#ifndef ITUNESDATABOX_H
#define ITUNESDATABOX_H

#include "mp4isofullbox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class ITunesDataBox: public Mp4IsoFullBox
    {
    public:
      ITunesDataBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~ITunesDataBox();

      //! get the internal data, which can be txt or binary data as well
      ByteVector data() const;

    private:
      //! parse the content of the box
      virtual void parse();

    protected:
      class ITunesDataBoxPrivate;
      ITunesDataBoxPrivate* d;
    }; // class ITunesDataBox
    
  } // namespace MP4
} // namespace TagLib

#endif // ITUNESDATABOX_H
