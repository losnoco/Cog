#include "mp4isobox.h"
#include "tfile.h"

using namespace TagLib;

class MP4::Mp4IsoBox::Mp4IsoBoxPrivate
{
public:
  MP4::Fourcc   fourcc;
  TagLib::uint  size;
  long          offset;
  TagLib::File* file;
};

MP4::Mp4IsoBox::Mp4IsoBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
{
  d = new MP4::Mp4IsoBox::Mp4IsoBoxPrivate();
  d->file   = file;
  d->fourcc = fourcc;
  d->size   = size;
  d->offset = offset;
}

MP4::Mp4IsoBox::~Mp4IsoBox()
{
  delete d;
}

void MP4::Mp4IsoBox::parsebox()
{
  // seek to offset
  file()->seek( offset(), File::Beginning );
  // simply call parse method of sub class
  parse();
}

MP4::Fourcc MP4::Mp4IsoBox::fourcc() const
{
  return d->fourcc;
}

TagLib::uint MP4::Mp4IsoBox::size() const
{
  return d->size;
}

long MP4::Mp4IsoBox::offset() const
{
  return d->offset;
}

TagLib::File* MP4::Mp4IsoBox::file() const
{
  return d->file;
}
