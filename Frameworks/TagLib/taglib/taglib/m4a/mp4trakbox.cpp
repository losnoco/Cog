#include "tlist.h"
#include <iostream>
#include "mp4trakbox.h"
#include "boxfactory.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4TrakBox::Mp4TrakBoxPrivate
{
public:
  //! container for all boxes in trak box
  TagLib::List<Mp4IsoBox*> trakBoxes;
  //! a box factory for creating the appropriate boxes
  MP4::BoxFactory        boxfactory;
}; // class Mp4TrakBoxPrivate

MP4::Mp4TrakBox::Mp4TrakBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4TrakBox::Mp4TrakBoxPrivate();
}

MP4::Mp4TrakBox::~Mp4TrakBox()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->trakBoxes.begin();
       delIter != d->trakBoxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

void MP4::Mp4TrakBox::parse()
{
  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  TagLib::uint totalsize = 8;
  // parse all contained boxes
  TagLib::uint size;
  MP4::Fourcc  fourcc;

  while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
  {
    totalsize += size;

    // check for errors
    if( totalsize > MP4::Mp4IsoBox::size() )
    {
      std::cerr << "Error in mp4 file " << mp4file->name() << " trak box contains bad box with name: " << fourcc.toString() << std::endl;
      return;
    }

    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( mp4file, fourcc, size, mp4file->tell() );
    curbox->parsebox();
    d->trakBoxes.append( curbox );

    // check for end of trak box
    if( totalsize == MP4::Mp4IsoBox::size() )
      break;
  }
}
