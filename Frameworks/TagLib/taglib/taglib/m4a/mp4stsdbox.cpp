#include "tlist.h"
#include <iostream>
#include "mp4stsdbox.h"
#include "mp4audiosampleentry.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4StsdBox::Mp4StsdBoxPrivate
{
public:
  //! the handler type for the current trak
  MP4::Fourcc   handler_type;
  //! the audio sample entry
  MP4::Mp4AudioSampleEntry* audioSampleEntry;
}; // class Mp4StsdBoxPrivate

MP4::Mp4StsdBox::Mp4StsdBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoFullBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4StsdBox::Mp4StsdBoxPrivate();
}

MP4::Mp4StsdBox::~Mp4StsdBox()
{
  delete d;
}

void MP4::Mp4StsdBox::setHandlerType( MP4::Fourcc fourcc )
{
  d->handler_type = fourcc;
}

void MP4::Mp4StsdBox::parse()
{
  MP4::File* mp4file = static_cast<MP4::File*>( file() );
  if(!mp4file)
    return;

  TagLib::uint totalsize = 12; // initial size of box

  // check for handler type - only parse if 'soun':
  if( static_cast<TagLib::uint>(d->handler_type) == 0x736f756e )
  {
    // read entry count
    TagLib::uint entry_count;
    if(!mp4file->readInt( entry_count ))
      return;

    // simply read first entry and skip all following
    // read size and type
    TagLib::uint cursize;
    MP4::Fourcc  fourcc;
    if( !mp4file->readSizeAndType( cursize, fourcc ))
      return;

    totalsize += 12;
    // alocate an AudioSampleEntry
    d->audioSampleEntry = new MP4::Mp4AudioSampleEntry( mp4file, fourcc, cursize, mp4file->tell() );
    // parse the AudioSampleEntry
    d->audioSampleEntry->parse();
    totalsize += cursize-8;
    // skip the remaining box contents
    mp4file->seek( size()-totalsize, TagLib::File::Current );
  }
  else
  {
    mp4file->seek( size()-totalsize, TagLib::File::Current );
  }
}
