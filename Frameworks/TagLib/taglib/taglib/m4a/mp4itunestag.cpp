#include "mp4itunestag.h"

using namespace TagLib;

class MP4::Tag::TagPrivate
{
public:
  MP4::File*         mp4file;
  TagLib::String     title;
  TagLib::String     artist;
  TagLib::String     album;
  TagLib::String     genre;
  TagLib::uint       year;
  TagLib::uint       track;
  TagLib::uint       numTracks;
  TagLib::String     comment;
  TagLib::String     grouping;
  TagLib::String     composer;
  TagLib::uint       disk;
  TagLib::uint       numDisks;
  TagLib::uint       bpm;
  float              rgAlbumGain;
  float              rgAlbumPeak;
  float              rgTrackGain;
  float              rgTrackPeak;
  bool               isEmpty;
  TagLib::ByteVector cover;
};


MP4::Tag::Tag( )
{
  d = new TagPrivate();
  d->year       = 0;
  d->track      = 0;
  d->numTracks  = 0;
  d->disk       = 0;
  d->numTracks  = 0;
  d->bpm        = 0;
  d->isEmpty    = true;
}

MP4::Tag::~Tag()
{
  delete d;
}

String MP4::Tag::title() const
{
  return d->title;
}

String MP4::Tag::artist() const
{
  return d->artist;
}

String MP4::Tag::album() const
{
  return d->album;
}

String MP4::Tag::comment() const
{
  return d->comment;
}

String MP4::Tag::genre() const
{
  return d->genre;
}

TagLib::uint MP4::Tag::year() const
{
  return d->year;
}

TagLib::uint MP4::Tag::track() const
{
  return d->track;
}

float MP4::Tag::rgAlbumGain() const
{
  return d->rgAlbumGain;
}

float MP4::Tag::rgAlbumPeak() const
{
  return d->rgAlbumPeak;
}

float MP4::Tag::rgTrackGain() const
{
  return d->rgTrackGain;
}

float MP4::Tag::rgTrackPeak() const
{
  return d->rgTrackPeak;
}

TagLib::uint MP4::Tag::numTracks() const
{
  return d->numTracks;
}

String MP4::Tag::grouping() const
{
  return d->grouping;
}

String MP4::Tag::composer() const
{
  return d->composer;
}

TagLib::uint MP4::Tag::disk() const
{
  return d->disk;
}

TagLib::uint MP4::Tag::numDisks() const
{
  return d->numDisks;
}

TagLib::uint MP4::Tag::bpm() const
{
  return d->bpm;
}

TagLib::ByteVector MP4::Tag::cover() const
{
  return d->cover;
}

void MP4::Tag::setTitle(const String &s)
{
  d->title = s;
  d->isEmpty = false;
}

void MP4::Tag::setArtist(const String &s)
{
  d->artist = s;
  d->isEmpty = false;
}

void MP4::Tag::setAlbum(const String &s)
{
  d->album = s;
  d->isEmpty = false;
}

void MP4::Tag::setComment(const String &s)
{
  d->comment = s;
  d->isEmpty = false;
}

void MP4::Tag::setGenre(const String &s)
{
  d->genre = s;
  d->isEmpty = false;
}

void MP4::Tag::setYear(TagLib::uint i)
{
  d->year = i;
  d->isEmpty = false;
}

void MP4::Tag::setTrack(TagLib::uint i)
{
  d->track = i;
  d->isEmpty = false;
}

void MP4::Tag::setRGAlbumGain(float f)
{
  d->rgAlbumGain = f;
  d->isEmpty = false;
}

void MP4::Tag::setRGAlbumPeak(float f)
{
    d->rgAlbumPeak = f;
    d->isEmpty = false;
}

void MP4::Tag::setRGTrackGain(float f)
{
    d->rgTrackGain = f;
    d->isEmpty = false;
}

void MP4::Tag::setRGTrackPeak(float f)
{
    d->rgTrackPeak = f;
    d->isEmpty = false;
}

void MP4::Tag::setNumTracks(TagLib::uint i)
{
  d->numTracks = i;
  d->isEmpty = false;
}

void MP4::Tag::setGrouping(const String &s)
{
  d->grouping = s;
  d->isEmpty = false;
}

void MP4::Tag::setComposer(const String &s)
{
  d->composer = s;
  d->isEmpty = false;
}

void MP4::Tag::setDisk(const TagLib::uint i)
{
  d->disk = i;
  d->isEmpty = false;
}

void MP4::Tag::setNumDisks(const TagLib::uint i)
{
  d->numDisks = i;
  d->isEmpty = false;
}

void MP4::Tag::setBpm(const TagLib::uint i)
{
  d->bpm = i;
  d->isEmpty = false;
}

void MP4::Tag::setCover(const TagLib::ByteVector& c)
{
  d->cover = c;
  d->isEmpty = false;
}

bool MP4::Tag::isEmpty() const
{
  return d->isEmpty;
}

