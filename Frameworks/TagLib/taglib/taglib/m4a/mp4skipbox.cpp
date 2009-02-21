#include "mp4skipbox.h"
#include "mp4isobox.h"
#include "tfile.h"

using namespace TagLib;

class MP4::Mp4SkipBox::Mp4SkipBoxPrivate
{
public:
};

MP4::Mp4SkipBox::Mp4SkipBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
	:Mp4IsoBox(file, fourcc, size, offset)
{
  d = new MP4::Mp4SkipBox::Mp4SkipBoxPrivate();
}

MP4::Mp4SkipBox::~Mp4SkipBox()
{
  delete d;
}

//! parse the content of the box
void MP4::Mp4SkipBox::parse()
{
  // skip contents
  file()->seek( size() - 8, TagLib::File::Current );
}

