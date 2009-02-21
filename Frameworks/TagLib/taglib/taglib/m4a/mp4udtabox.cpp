#include "tlist.h"
#include <iostream>
#include "mp4udtabox.h"
#include "boxfactory.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4UdtaBox::Mp4UdtaBoxPrivate
{
public:
  //! container for all boxes in udta box
  TagLib::List<Mp4IsoBox*> udtaBoxes;
  //! a box factory for creating the appropriate boxes
  MP4::BoxFactory        boxfactory;
}; // class Mp4UdtaBoxPrivate

MP4::Mp4UdtaBox::Mp4UdtaBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4UdtaBox::Mp4UdtaBoxPrivate();
}

MP4::Mp4UdtaBox::~Mp4UdtaBox()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->udtaBoxes.begin();
       delIter != d->udtaBoxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

void MP4::Mp4UdtaBox::parse()
{
#if 0
  std::cout << "  parsing udta box" << std::endl;
#endif
  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  TagLib::uint totalsize = 8;
  // parse all contained boxes
  TagLib::uint size;
  MP4::Fourcc  fourcc;

#if 0
  std::cout << "    ";
#endif
  while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
  {
    totalsize += size;

    // check for errors
    if( totalsize > MP4::Mp4IsoBox::size() )
    {
      std::cerr << "Error in mp4 file " << mp4file->name() << " udta box contains bad box with name: " << fourcc.toString() << std::endl;
      return;
    }

    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( mp4file, fourcc, size, mp4file->tell() );
    curbox->parsebox();
    d->udtaBoxes.append( curbox );

    // check for end of udta box
    if( totalsize == MP4::Mp4IsoBox::size() )
      break;
#if 0
    std::cout << "    ";
#endif
  }
}
