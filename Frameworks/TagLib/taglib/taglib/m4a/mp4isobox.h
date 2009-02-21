#ifndef MP4ISOBOX_H
#define MP4ISOBOX_H

#include "taglib.h"
#include "mp4fourcc.h"

namespace TagLib
{
  class File; 

  namespace MP4
  {
    class Mp4IsoBox
    {
    public:
      //! constructor for base class
      Mp4IsoBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset );
      //! destructor - simply freeing private ptr
      virtual ~Mp4IsoBox();

      //! function to get the fourcc code
      MP4::Fourcc fourcc() const;
      //! function to get the size of tha atom/box
      uint size() const;
      //! function to get the offset of the atom in the mp4 file
      long offset() const;

      //! parse wrapper to get common interface for both box and fullbox
      virtual void  parsebox();
      //! pure virtual function for all subclasses to implement
      virtual void parse() = 0;

    protected:
      //! function to get the file pointer
      TagLib::File* file() const;

    protected:
      class Mp4IsoBoxPrivate;
      Mp4IsoBoxPrivate* d;
    };

  } // namespace MP4
} // namespace TagLib

#endif // MP4ISOBOX_H

