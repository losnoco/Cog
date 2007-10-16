#ifndef MP4SAMPLEENTRY_H
#define MP4SAMPLEENTRY_H

#include "mp4isobox.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4SampleEntry: public Mp4IsoBox
    {
    public:
      Mp4SampleEntry( File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~Mp4SampleEntry();

    public:
      //! parse the content of the box
      virtual void parse();

    private:
      //! function to be implemented in subclass
      virtual void parseEntry() = 0;

    protected:
      class Mp4SampleEntryPrivate;
      Mp4SampleEntryPrivate* d;
    }; // class Mp4SampleEntry
    
  } // namespace MP4
} // namespace TagLib

#endif // MP4SAMPLEENTRY_H
