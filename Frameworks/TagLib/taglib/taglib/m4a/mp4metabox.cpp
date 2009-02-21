#include <tlist.h>
#include <iostream>
#include "mp4metabox.h"
#include "boxfactory.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4MetaBox::Mp4MetaBoxPrivate
{
public:
  //! container for all boxes in meta box
  TagLib::List<Mp4IsoBox*> metaBoxes;
  //! a box factory for creating the appropriate boxes
  MP4::BoxFactory        boxfactory;
}; // class Mp4MetaBoxPrivate

MP4::Mp4MetaBox::Mp4MetaBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoFullBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4MetaBox::Mp4MetaBoxPrivate();
}

MP4::Mp4MetaBox::~Mp4MetaBox()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->metaBoxes.begin();
       delIter != d->metaBoxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

void MP4::Mp4MetaBox::parse()
{
  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  TagLib::uint totalsize = 12; // initial size of box
  // parse all contained boxes
  TagLib::uint size;
  MP4::Fourcc  fourcc;

  while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
  {
    totalsize += size;

    // check for errors
    if( totalsize > MP4::Mp4IsoBox::size() )
    {
      std::cerr << "Error in mp4 file " << mp4file->name() << " meta box contains bad box with name: " << fourcc.toString() << std::endl;
      return;
    }

    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( mp4file, fourcc, size, mp4file->tell() );
    curbox->parsebox();
    d->metaBoxes.append( curbox );

    // check for end of meta box
    if( totalsize == MP4::Mp4IsoBox::size() )
      break;
  }
}
