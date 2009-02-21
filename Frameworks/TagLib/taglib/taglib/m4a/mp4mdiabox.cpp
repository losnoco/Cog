#include "tlist.h"
#include <iostream>
#include "mp4mdiabox.h"
#include "mp4hdlrbox.h"
#include "mp4minfbox.h"
#include "boxfactory.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4MdiaBox::Mp4MdiaBoxPrivate
{
public:
  //! container for all boxes in mdia box
  TagLib::List<Mp4IsoBox*> mdiaBoxes;
  //! a box factory for creating the appropriate boxes
  MP4::BoxFactory        boxfactory;
}; // class Mp4MdiaBoxPrivate

MP4::Mp4MdiaBox::Mp4MdiaBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4MdiaBox::Mp4MdiaBoxPrivate();
}

MP4::Mp4MdiaBox::~Mp4MdiaBox()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->mdiaBoxes.begin();
       delIter != d->mdiaBoxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

void MP4::Mp4MdiaBox::parse()
{
  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  TagLib::uint totalsize = 8;
  // parse all contained boxes
  TagLib::uint size;
  MP4::Fourcc  fourcc;

  // stores the current handler type
  TagLib::MP4::Fourcc hdlrtype;

  while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
  {
    totalsize += size;

    // check for errors
    if( totalsize > MP4::Mp4IsoBox::size() )
    {
      std::cerr << "Error in mp4 file " << mp4file->name() << " mdia box contains bad box with name: " << fourcc.toString() << std::endl;
      return;
    }

    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( mp4file, fourcc, size, mp4file->tell() );
    if( static_cast<TagLib::uint>( fourcc ) == 0x6d696e66 /*"minf"*/ )
    {
      // cast to minf
      Mp4MinfBox* minfbox = static_cast<Mp4MinfBox*>( curbox );
      if(!minfbox)
	return;
      // set handler type
      minfbox->setHandlerType( hdlrtype );
    }

    curbox->parsebox();
    d->mdiaBoxes.append( curbox );

    if(static_cast<TagLib::uint>( fourcc ) == 0x68646c72 /*"hdlr"*/ )
    {
      // cast to hdlr box
      Mp4HdlrBox* hdlrbox = static_cast<Mp4HdlrBox*>( curbox );
      if(!hdlrbox)
	return;
      // get handler type
      hdlrtype = hdlrbox->hdlr_type();
    }
    // check for end of mdia box
    if( totalsize == MP4::Mp4IsoBox::size() )
      break;

  }
}
