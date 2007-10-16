#include <iostream>
#include "mp4audiosampleentry.h"
#include "mp4isobox.h"
#include "mp4file.h"
#include "mp4propsproxy.h"

using namespace TagLib;

class MP4::Mp4AudioSampleEntry::Mp4AudioSampleEntryPrivate
{
public:
  TagLib::uint channelcount;
  TagLib::uint samplerate;
  TagLib::uint bitrate;
};

MP4::Mp4AudioSampleEntry::Mp4AudioSampleEntry( TagLib::File* file, MP4::Fourcc fourcc, uint size, long offset )
	:Mp4SampleEntry(file, fourcc, size, offset)
{
  d = new MP4::Mp4AudioSampleEntry::Mp4AudioSampleEntryPrivate();
}

MP4::Mp4AudioSampleEntry::~Mp4AudioSampleEntry()
{
  delete d;
}

TagLib::uint MP4::Mp4AudioSampleEntry::channels() const
{
  return d->channelcount;
}

TagLib::uint MP4::Mp4AudioSampleEntry::samplerate() const
{
  return d->samplerate;
}

TagLib::uint MP4::Mp4AudioSampleEntry::bitrate() const
{
  return d->bitrate;
}

void MP4::Mp4AudioSampleEntry::parseEntry()
{
  TagLib::MP4::File* mp4file = static_cast<TagLib::MP4::File*>(file());
  if(!mp4file)
    return;

  // read 8 reserved bytes
  mp4file->seek( 8, TagLib::File::Current );
  // read channelcount
  if(!mp4file->readShort( d->channelcount ))
    return;
  // seek over samplesize, pre_defined and reserved
  mp4file->seek( 6, TagLib::File::Current );
  // read samplerate
  if(!mp4file->readInt( d->samplerate ))
    return;

  // register box at proxy
  mp4file->propProxy()->registerAudioSampleEntry( this );


  //std::cout << "fourcc of audio sample entry: " << fourcc().toString() << std::endl;
  // check for both mp4a (plain files) and drms (encrypted files)
  if( (fourcc() == MP4::Fourcc("mp4a")) ||
      (fourcc() == MP4::Fourcc("drms"))  )
  {
    TagLib::MP4::Fourcc fourcc;
    TagLib::uint        esds_size;

    mp4file->readSizeAndType( esds_size, fourcc );

    // read esds' main parts
    if( size()-48 > 0 )
      ByteVector flags_version = mp4file->readBlock(4);
    else
      return;

    ByteVector EsDescrTag = mp4file->readBlock(1);
    // first 4 bytes contain full box specifics (version & flags)
    // upcoming byte must be ESDescrTag (0x03)
    if( EsDescrTag[0] == 0x03 )
    {
      TagLib::uint descr_len = mp4file->readSystemsLen();
      TagLib::uint EsId;
      if( !mp4file->readShort( EsId ) )
	return;
      ByteVector priority = mp4file->readBlock(1);
      if( descr_len < 20 )
	return;
    }
    else
    {
      TagLib::uint EsId;
      if( !mp4file->readShort( EsId ) )
	return;
    }
    // read decoder configuration tag (0x04)
    ByteVector DecCfgTag = mp4file->readBlock(1);
    if( DecCfgTag[0] != 0x04 )
      return;
    // read decoder configuration length
    TagLib::uint deccfg_len = mp4file->readSystemsLen();
    // read object type Id
    ByteVector objId = mp4file->readBlock(1);
    // read stream type id
    ByteVector strId = mp4file->readBlock(1);
    // read buffer Size DB
    ByteVector bufferSizeDB = mp4file->readBlock(3);
    // read max bitrate
    TagLib::uint max_bitrate;
    if( !mp4file->readInt( max_bitrate ) )
      return;
    // read average bitrate
    if( !mp4file->readInt( d->bitrate ) )
      return;
    // skip the rest
    mp4file->seek( offset()+size()-8, File::Beginning );
  }
  else
    mp4file->seek( size()-36, File::Current );
}

