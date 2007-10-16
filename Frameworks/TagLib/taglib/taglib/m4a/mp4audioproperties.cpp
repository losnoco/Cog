#include "mp4audioproperties.h"
#include "mp4propsproxy.h"

using namespace TagLib;

class MP4::AudioProperties::AudioPropertiesPrivate
{
public:
  MP4::Mp4PropsProxy* propsproxy;
}; // AudioPropertiesPrivate

MP4::AudioProperties::AudioProperties():TagLib::AudioProperties(TagLib::AudioProperties::Average)
{
  d = new MP4::AudioProperties::AudioPropertiesPrivate();
}

MP4::AudioProperties::~AudioProperties()
{
  delete d;
}

void MP4::AudioProperties::setProxy( Mp4PropsProxy* proxy )
{
  d->propsproxy = proxy;
}

int MP4::AudioProperties::length() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->seconds();
}

int MP4::AudioProperties::bitrate() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->bitRate()/1000;
}

int MP4::AudioProperties::sampleRate() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->sampleRate();
}

int MP4::AudioProperties::channels() const
{
  if( d->propsproxy == 0 )
    return 0;
  return d->propsproxy->channels();
}

