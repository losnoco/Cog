#include "mp4isofullbox.h"
#include "tfile.h"

using namespace TagLib;

class MP4::Mp4IsoFullBox::Mp4IsoFullBoxPrivate
{
public:
	uchar version;
	uint  flags;
}; // Mp4IsoFullBoxPrivate


MP4::Mp4IsoFullBox::Mp4IsoFullBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
: Mp4IsoBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4IsoFullBox::Mp4IsoFullBoxPrivate();
}

MP4::Mp4IsoFullBox::~Mp4IsoFullBox()
{
  delete d;
}

void  MP4::Mp4IsoFullBox::parsebox()
{
  // seek to offset
  Mp4IsoBox::file()->seek(Mp4IsoBox::offset(), File::Beginning );
  // parse version and flags
  ByteVector version_flags = Mp4IsoBox::file()->readBlock(4);
  d->version = version_flags[0];
  d->flags   = version_flags[1] << 16 || version_flags[2] << 8 || version_flags[3];
  // call parse method of subclass
  parse();
}

TagLib::uchar MP4::Mp4IsoFullBox::version()
{
  return d->version;
}

TagLib::uint  MP4::Mp4IsoFullBox::flags()
{
  return d->flags;
}

