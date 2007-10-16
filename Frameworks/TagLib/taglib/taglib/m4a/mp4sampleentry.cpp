#include "mp4sampleentry.h"
#include "mp4isobox.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4SampleEntry::Mp4SampleEntryPrivate
{
public:
  TagLib::uint data_reference_index;
};

MP4::Mp4SampleEntry::Mp4SampleEntry( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
	:Mp4IsoBox(file, fourcc, size, offset)
{
  d = new MP4::Mp4SampleEntry::Mp4SampleEntryPrivate();
}

MP4::Mp4SampleEntry::~Mp4SampleEntry()
{
  delete d;
}

//! parse the content of the box
void MP4::Mp4SampleEntry::parse()
{
  TagLib::MP4::File* mp4file = static_cast<TagLib::MP4::File*>(file());
  if(!mp4file)
    return;

  // skip the first 6 bytes
  mp4file->seek( 6, TagLib::File::Current );
  // read data reference index
  if(!mp4file->readShort( d->data_reference_index))
    return;
  parseEntry();
}

