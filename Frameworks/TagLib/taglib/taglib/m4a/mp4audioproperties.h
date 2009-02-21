#ifndef MP4AUDIOPROPERTIES_H
#define MP4AUDIOPROPERTIES_H MP4AUDIOPROPERTIES_H

#include "audioproperties.h"

namespace TagLib
{
  namespace MP4
  {
    class Mp4PropsProxy;

    class AudioProperties : public TagLib::AudioProperties
    {
    public:
      //! constructor
      AudioProperties();
      //! destructor
      ~AudioProperties();

      //! function to set the proxy
      void setProxy( Mp4PropsProxy* proxy );

      /*!
       * Returns the lenght of the file in seconds.
       */
      int length() const;

      /*!
       * Returns the most appropriate bit rate for the file in kb/s.  For constant
       * bitrate formats this is simply the bitrate of the file.  For variable
       * bitrate formats this is either the average or nominal bitrate.
       */
      int bitrate() const;

      /*!
       * Returns the sample rate in Hz.
       */
      int sampleRate() const;

      /*!
       * Returns the number of audio channels.
       */
      int channels() const;

    private:
      class AudioPropertiesPrivate;
      AudioPropertiesPrivate* d;
    };
  } // namespace MP4
} // namespace TagLib

#endif // MP4AUDIOPROPERTIES_H
