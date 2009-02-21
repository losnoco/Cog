#ifndef MP4MVHDBOX_H
#define MP4MVHDBOX_H

#include "mp4isofullbox.h"
#include "mp4fourcc.h"
#include "mp4file.h" // ulonglong

namespace TagLib
{
  namespace MP4
  {
    class Mp4MvhdBox: public Mp4IsoFullBox
    {
    public:
      Mp4MvhdBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset );
      ~Mp4MvhdBox();

      //! function to get the creation time of the mp4 file
      ulonglong creationTime() const;
      //! function to get the modification time of the mp4 file
      ulonglong modificationTime() const;
      //! function to get the timescale referenced by the above timestamps
      uint timescale() const;
      //! function to get the presentation duration in the mp4 file
      ulonglong duration() const;
      //! function to get the rate (playout speed) - typically 1.0;
      uint rate() const;
      //! function to get volume level for presentation - typically 1.0;
      uint volume() const;
      //! function to get the track ID for adding new tracks - useless for this lib
      uint nextTrackID() const;

      //! parse mvhd contents
      void parse();

    private:
      class Mp4MvhdBoxPrivate;
      Mp4MvhdBoxPrivate* d;
    }; // Mp4MvhdBox

  } // namespace MP4
} // namespace TagLib

#endif // MP4MVHDBOX_H
