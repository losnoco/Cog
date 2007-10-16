#include <iostream>
#include "itunestrknbox.h"
#include "itunesdatabox.h"
#include "mp4isobox.h"
#include "mp4file.h"
#include "tfile.h"
#include "mp4tagsproxy.h"

using namespace TagLib;

class MP4::ITunesTrknBox::ITunesTrknBoxPrivate
{
public:
  ITunesDataBox* dataBox;
};

MP4::ITunesTrknBox::ITunesTrknBox( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
	:Mp4IsoBox(file, fourcc, size, offset)
{
  d = new MP4::ITunesTrknBox::ITunesTrknBoxPrivate();
  d->dataBox = 0;
}

MP4::ITunesTrknBox::~ITunesTrknBox()
{
  if( d->dataBox != 0 )
    delete d->dataBox;
  delete d;
}

//! parse the content of the box
void MP4::ITunesTrknBox::parse()
{
  TagLib::MP4::File* mp4file = static_cast<MP4::File*>( file() );

  // parse data box
  TagLib::uint size;
  MP4::Fourcc  fourcc;

  if(mp4file->readSizeAndType( size, fourcc ) == true)  
  {
    // check for type - must be 'data'
    if( fourcc != MP4::Fourcc("data") )
    {
      std::cerr << "bad atom in itunes tag - skipping it." << std::endl; 
      // jump over data tag
      mp4file->seek( size-8, TagLib::File::Current );
      return;
    }
    d->dataBox = new ITunesDataBox( mp4file, fourcc, size, mp4file->tell() );
    d->dataBox->parsebox();
  }
  else
  {
    // reading unsuccessful - serious error!
    std::cerr << "Error in parsing ITunesTrknBox - serious Error in taglib!" << std::endl;
    return;
  }
  // register data box
  mp4file->tagProxy()->registerBox( Mp4TagsProxy::trackno, d->dataBox );

#if 0
  // get data pointer - just for debuging...
  TagLib::ByteVector trknData = d->dataBox->data();
  TagLib::String trknumber = TagLib::String::number( static_cast<int>( static_cast<unsigned char>(trknData[0]) << 24 |
	                                                               static_cast<unsigned char>(trknData[1]) << 16 |
								       static_cast<unsigned char>(trknData[2]) <<  8 |
								       static_cast<unsigned char>(trknData[3]) ) );
  std::cout << "Content of tracknumber box: " << trknumber << std::endl;
#endif
}

