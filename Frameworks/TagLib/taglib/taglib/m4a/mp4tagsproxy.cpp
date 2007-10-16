#include "mp4tagsproxy.h"
#include "itunesdatabox.h"

using namespace TagLib;

class MP4::Mp4TagsProxy::Mp4TagsProxyPrivate
{
public:
  ITunesDataBox* titleData;
  ITunesDataBox* artistData;
  ITunesDataBox* albumData;
  ITunesDataBox* coverData;
  ITunesDataBox* genreData;
  ITunesDataBox* yearData;
  ITunesDataBox* trknData;
  ITunesDataBox* commentData;
  ITunesDataBox* groupingData;
  ITunesDataBox* composerData;
  ITunesDataBox* diskData;
  ITunesDataBox* bpmData;
}; 

MP4::Mp4TagsProxy::Mp4TagsProxy()
{
  d = new MP4::Mp4TagsProxy::Mp4TagsProxyPrivate();
  d->titleData    = 0;
  d->artistData   = 0;
  d->albumData    = 0;
  d->coverData    = 0;
  d->genreData    = 0;
  d->yearData     = 0;
  d->trknData     = 0;
  d->commentData  = 0;
  d->groupingData = 0;
  d->composerData = 0;
  d->diskData     = 0;
  d->bpmData      = 0;
}

MP4::Mp4TagsProxy::~Mp4TagsProxy()
{
  delete d;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::titleData() const
{
  return d->titleData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::artistData() const
{
  return d->artistData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::albumData() const
{
  return d->albumData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::genreData() const
{
  return d->genreData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::yearData() const
{
  return d->yearData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::trknData() const
{
  return d->trknData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::commentData() const
{
  return d->commentData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::groupingData() const
{
  return d->groupingData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::composerData() const
{
  return d->composerData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::diskData() const
{
  return d->diskData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::bpmData() const
{
  return d->bpmData;
}

MP4::ITunesDataBox* MP4::Mp4TagsProxy::coverData() const
{
  return d->coverData;
}

void MP4::Mp4TagsProxy::registerBox( EBoxType boxtype, ITunesDataBox* databox )
{
  switch( boxtype )
  {
    case title:
      d->titleData = databox;
      break;
    case artist:
      d->artistData = databox;
      break;
    case album:
      d->albumData = databox;
      break;
    case cover:
      d->coverData = databox;
      break;
    case genre:
      d->genreData = databox;
      break;
    case year:
      d->yearData = databox;
      break;
    case trackno:
      d->trknData = databox;
      break;
    case comment:
      d->commentData = databox;
      break;
    case grouping:
      d->groupingData = databox;
      break;
    case composer:
      d->composerData = databox;
      break;
    case disk:
      d->diskData = databox;
      break;
    case bpm:
      d->bpmData = databox;
      break;
  }
}

