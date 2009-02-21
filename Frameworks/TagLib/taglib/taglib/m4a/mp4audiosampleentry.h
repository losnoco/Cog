#ifndef MP4AUDIOSAMPLEENTRY_H
#define MP4AUDIOSAMPLEENTRY_H

#include "mp4sampleentry.h"
#include "mp4fourcc.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4AudioSampleEntry: public Mp4SampleEntry
    {
    public:
      Mp4AudioSampleEntry( File* file, MP4::Fourcc fourcc, uint size, long offset );
      ~Mp4AudioSampleEntry();

      //! function to get the number of channels
      TagLib::uint channels() const;
      //! function to get the sample rate
      TagLib::uint samplerate() const;
      //! function to get the average bitrate of the audio stream
      TagLib::uint bitrate() const;

    private:
      //! parse the content of the box
      void parseEntry();

    protected:
      class Mp4AudioSampleEntryPrivate;
      Mp4AudioSampleEntryPrivate* d;
    }; // class Mp4AudioSampleEntry
    
  } // namespace MP4
} // namespace TagLib

#endif // MP4AUDIOSAMPLEENTRY_H
