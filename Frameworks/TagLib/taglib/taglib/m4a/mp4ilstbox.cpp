#include "tlist.h"
#include <iostream>
#include "mp4ilstbox.h"
#include "boxfactory.h"
#include "mp4file.h"

using namespace TagLib;

class MP4::Mp4IlstBox::Mp4IlstBoxPrivate
{
public:
  //! container for all boxes in ilst box
  TagLib::List<Mp4IsoBox*> ilstBoxes;
  //! a box factory for creating the appropriate boxes
  MP4::BoxFactory        boxfactory;
}; // class Mp4IlstBoxPrivate

MP4::Mp4IlstBox::Mp4IlstBox( TagLib::File* file, MP4::Fourcc fourcc, TagLib::uint size, long offset )
	: Mp4IsoBox( file, fourcc, size, offset )
{
  d = new MP4::Mp4IlstBox::Mp4IlstBoxPrivate();
}

MP4::Mp4IlstBox::~Mp4IlstBox()
{
  TagLib::List<Mp4IsoBox*>::Iterator delIter;
  for( delIter  = d->ilstBoxes.begin();
       delIter != d->ilstBoxes.end();
       delIter++ )
  {
    delete *delIter;
  }
  delete d;
}

void MP4::Mp4IlstBox::parse()
{
#if 0
  std::cout << "      parsing ilst box" << std::endl;
#endif

  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  TagLib::uint totalsize = 8;
  // parse all contained boxes
  TagLib::uint size;
  MP4::Fourcc  fourcc;

#if 0
  std::cout << "      ";
#endif
  while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
  {
    totalsize += size;

    // check for errors
    if( totalsize > MP4::Mp4IsoBox::size() )
    {
      std::cerr << "Error in mp4 file " << mp4file->name() << " ilst box contains bad box with name: " << fourcc.toString() << std::endl;
      return;
    }

    // create the appropriate subclass and parse it
    MP4::Mp4IsoBox* curbox = d->boxfactory.createInstance( mp4file, fourcc, size, mp4file->tell() );
    curbox->parsebox();
    d->ilstBoxes.append( curbox );

    // check for end of ilst box
    if( totalsize == MP4::Mp4IsoBox::size() )
      break;

#if 0
    std::cout << "      ";
#endif
  }
}
