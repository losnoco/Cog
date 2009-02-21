#include <deque>
#include <iostream>
#include "mp4mvhdbox.h"
#include "boxfactory.h"
#include "mp4file.h"
#include "mp4propsproxy.h"

using namespace TagLib;

class MP4::Mp4MvhdBox::Mp4MvhdBoxPrivate
{
public:
  //! creation time of the file
  TagLib::ulonglong creationTime;
  //! modification time of the file - since midnight, Jan. 1, 1904, UTC-time
  TagLib::ulonglong modificationTime;
  //! timescale for the file - referred by all time specifications in this box
  TagLib::uint      timescale;
  //! duration of presentation
  TagLib::ulonglong duration;
  //! playout speed
  TagLib::uint      rate;
  //! volume for entire presentation
  TagLib::uint      volume;
  //! track ID for an additional track (next new track)
  TagLib::uint      nextTrackID;
}; // class Mp4MvhdBoxPrivate

MP4::Mp4MvhdBox::Mp4MvhdBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
               : Mp4IsoFullBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4MvhdBox::Mp4MvhdBoxPrivate();
}

MP4::Mp4MvhdBox::~Mp4MvhdBox()
{
  delete d;
}

TagLib::ulonglong MP4::Mp4MvhdBox::creationTime() const
{
  return d->creationTime;
}

TagLib::ulonglong MP4::Mp4MvhdBox::modificationTime() const
{
  return d->modificationTime;
}

TagLib::uint      MP4::Mp4MvhdBox::timescale() const
{
  return d->timescale;
}

TagLib::ulonglong MP4::Mp4MvhdBox::duration() const
{
  return d->duration;
}

TagLib::uint      MP4::Mp4MvhdBox::rate() const
{
  return d->rate;
}

TagLib::uint      MP4::Mp4MvhdBox::volume() const
{
  return d->volume;
}

TagLib::uint      MP4::Mp4MvhdBox::nextTrackID() const
{
  return d->nextTrackID;
}


void MP4::Mp4MvhdBox::parse()
{
  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  if( version() == 1 )
  {
    if( !mp4file->readLongLong( d->creationTime ) )
      return;
    if( !mp4file->readLongLong( d->modificationTime ) )
      return;
    if( !mp4file->readInt( d->timescale ) )
      return;
    if( !mp4file->readLongLong( d->duration ) )
      return;
  }
  else
  {
    TagLib::uint creationTime_tmp, modificationTime_tmp, duration_tmp;

    if( !mp4file->readInt( creationTime_tmp ) )
      return;
    if( !mp4file->readInt( modificationTime_tmp ) )
      return;
    if( !mp4file->readInt( d->timescale ) )
      return;
    if( !mp4file->readInt( duration_tmp ) )
      return;

    d->creationTime     = creationTime_tmp;
    d->modificationTime = modificationTime_tmp;
    d->duration         = duration_tmp;
  }
  if( !mp4file->readInt( d->rate ) )
    return;
  if( !mp4file->readInt( d->volume ) )
    return;
  // jump over unused fields
  mp4file->seek( 68, File::Current );

  if( !mp4file->readInt( d->nextTrackID ) )
    return;
  // register at proxy
  mp4file->propProxy()->registerMvhd( this );
}
