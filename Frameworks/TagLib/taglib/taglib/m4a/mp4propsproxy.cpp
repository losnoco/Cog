#include "mp4propsproxy.h"

using namespace TagLib;

class MP4::Mp4PropsProxy::Mp4PropsProxyPrivate
{
public:
  //! the movie header box
  MP4::Mp4MvhdBox* mvhdbox;
  //! the sample table box
  MP4::Mp4AudioSampleEntry* audiosampleentry;
};


MP4::Mp4PropsProxy::Mp4PropsProxy()
{
  d = new MP4::Mp4PropsProxy::Mp4PropsProxyPrivate();
  d->mvhdbox = 0;
  d->audiosampleentry = 0;
}

MP4::Mp4PropsProxy::~Mp4PropsProxy()
{
  delete d;
}

TagLib::uint MP4::Mp4PropsProxy::seconds() const
{
  if( d->mvhdbox )
    return static_cast<TagLib::uint>( d->mvhdbox->duration() / d->mvhdbox->timescale() );
  else
    return 0;
}

TagLib::uint MP4::Mp4PropsProxy::channels() const
{
  if( d->audiosampleentry )
    return d->audiosampleentry->channels();
  else
    return 0;
}

TagLib::uint MP4::Mp4PropsProxy::sampleRate() const
{
  if( d->audiosampleentry )
    return (d->audiosampleentry->samplerate()>>16);
  else
    return 0;
}

TagLib::uint MP4::Mp4PropsProxy::bitRate() const
{
  if( d->audiosampleentry )
    return (d->audiosampleentry->bitrate());
  else
    return 0;
}

void MP4::Mp4PropsProxy::registerMvhd( MP4::Mp4MvhdBox* mvhdbox )
{
  d->mvhdbox = mvhdbox;
}

void MP4::Mp4PropsProxy::registerAudioSampleEntry( MP4::Mp4AudioSampleEntry* audioSampleEntry )
{
  d->audiosampleentry = audioSampleEntry;
}

