#include <iostream>
#include "itunesdatabox.h"
#include "tbytevector.h"
#include "mp4isobox.h"
#include "tfile.h"

using namespace TagLib;

class MP4::ITunesDataBox::ITunesDataBoxPrivate
{
public:
  ByteVector data;
};

MP4::ITunesDataBox::ITunesDataBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
	:Mp4IsoFullBox(file, fourcc, size, offset)
{
  d = new MP4::ITunesDataBox::ITunesDataBoxPrivate();
}

MP4::ITunesDataBox::~ITunesDataBox()
{
  delete d;
}

ByteVector MP4::ITunesDataBox::data() const
{
  return d->data;
}

//! parse the content of the box
void MP4::ITunesDataBox::parse()
{
  // skip first 4 byte - don't know what they are supposed to be for - simply 4 zeros
  file()->seek( 4, TagLib::File::Current );
  // read contents - remaining size is box_size-12-4 (12:fullbox header, 4:starting zeros of data box)
#if 0
  std::cout << "           reading data box with data length: " << size()-16 << std::endl;
#endif
  d->data = file()->readBlock( size()-12-4 );
}

