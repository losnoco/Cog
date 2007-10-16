#include "tlist.h"
#include <iostream>
#include "mp4stblbox.h"
#include "mp4stsdbox.h"
#include "boxfactory.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4StblBox::Mp4StblBoxPrivate
{
public:
  //! container for all boxes in stbl box
  TagLib::List<Mp4IsoBox*> stblBoxes;
  //! a box factory for creating the appropriate boxes
  MP4::BoxFactory        boxfactory;
  //! the handler type for the current trak
  MP4::Fourcc            handler_type;
}; // class Mp4StblBoxPrivate

MP4::Mp4StblBox::Mp4StblBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4StblBox::Mp4StblBoxPrivate();
}

MP4::Mp4StblBox::~Mp4StblBox()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->stblBoxes.begin();
       delIter != d->stblBoxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

void MP4::Mp4StblBox::setHandlerType( MP4::Fourcc fourcc )
{
  d->handler_type = fourcc;
}

void MP4::Mp4StblBox::parse()
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
      std::cerr << "Error in mp4 file " << mp4file->name() << " stbl box contains bad box with name: " << fourcc.toString() << std::endl;
      return;
    }

    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( mp4file, fourcc, size, mp4file->tell() );

    // check for stsd
    if( static_cast<TagLib::uint>(fourcc) == 0x73747364 /*'stsd'*/ )
    {
      // cast to stsd box
      MP4::Mp4StsdBox* stsdbox = static_cast<MP4::Mp4StsdBox*>(curbox);
      if(!stsdbox)
	return;
      // set the handler type
      stsdbox->setHandlerType( d->handler_type );
    }
    curbox->parsebox();
    d->stblBoxes.append( curbox );

    // check for end of stbl box
    if( totalsize == MP4::Mp4IsoBox::size() )
      break;
  }
}
