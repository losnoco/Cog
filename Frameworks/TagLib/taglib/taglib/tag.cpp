/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include <taglib/tag.h>
#include <taglib/toolkit/tstringlist.h>
#include <taglib/toolkit/tpropertymap.h>

using namespace TagLib;

class Tag::TagPrivate
{

};

Tag::Tag()
{

}

Tag::~Tag()
{

}

bool Tag::isEmpty() const
{
  return (title().isEmpty() &&
          albumartist().isEmpty() &&
          artist().isEmpty() &&
          composer().isEmpty() &&
          album().isEmpty() &&
          unsyncedlyrics().isEmpty() &&
          comment().isEmpty() &&
          genre().isEmpty() &&
          year() == 0 &&
          track() == 0 &&
          disc() == 0);
}

PropertyMap Tag::properties() const
{
  PropertyMap map;
  if(!(title().isEmpty()))
    map["TITLE"].append(title());
  if(!(albumartist().isEmpty()))
    map["ALBUMARTIST"].append(albumartist());
  if(!(artist().isEmpty()))
    map["ARTIST"].append(artist());
  if(!(composer().isEmpty()))
    map["COMPOSER"].append(composer());
  if(!(album().isEmpty()))
    map["ALBUM"].append(album());
  if(!(unsyncedlyrics().isEmpty()))
    map["UNSYNCEDLYRICS"].append(unsyncedlyrics());
  if(!(comment().isEmpty()))
    map["COMMENT"].append(comment());
  if(!(genre().isEmpty()))
    map["GENRE"].append(genre());
  if(!(year() == 0))
    map["DATE"].append(String::number(year()));
  if(!(track() == 0))
    map["TRACKNUMBER"].append(String::number(track()));
  if (!(disc() == 0))
    map["DISCNUMBER"].append(String::number(disc()));
  return map;
}

void Tag::removeUnsupportedProperties(const StringList&)
{
}

PropertyMap Tag::setProperties(const PropertyMap &origProps)
{
  PropertyMap properties(origProps);
  properties.removeEmpty();
  StringList oneValueSet;
  // can this be simplified by using some preprocessor defines / function pointers?
  if(properties.contains("TITLE")) {
    setTitle(properties["TITLE"].front());
    oneValueSet.append("TITLE");
  } else
    setTitle(String());

  if(properties.contains("ALBUMARTIST") ||
     properties.contains("ALBUM ARTIST")) {
    if (properties.contains("ALBUMARTIST"))
      setAlbumArtist(properties["ALBUMARTIST"].front());
    else
      setAlbumArtist(properties["ALBUM ARTIST"].front());
    oneValueSet.append("ALBUMARTIST");
  } else
    setAlbumArtist(String());

  if(properties.contains("ARTIST")) {
    setArtist(properties["ARTIST"].front());
    oneValueSet.append("ARTIST");
  } else
    setArtist(String());

  if(properties.contains("COMPOSER")) {
    setComposer(properties["COMPOSER"].front());
    oneValueSet.append("COMPOSER");
  } else
    setComposer(String());

  if(properties.contains("ALBUM")) {
    setAlbum(properties["ALBUM"].front());
    oneValueSet.append("ALBUM");
  } else
    setAlbum(String());

  if(properties.contains("UNSYNCEDLYRICS") ||
     properties.contains("UNSYNCED LYRICS") ||
     properties.contains("LYRICS")) {
    if(properties.contains("UNSYNCEDLYRICS"))
      setUnsyncedlyrics(properties["UNSYNCEDLYRICS"].front());
    else if(properties.contains("UNSYNCED LYRICS"))
      setUnsyncedlyrics(properties["UNSYNCED LYRICS"].front());
    else
      setUnsyncedlyrics(properties["LYRICS"].front());
    oneValueSet.append("UNSYNCEDLYRICS");
  }

  if(properties.contains("COMMENT")) {
    setComment(properties["COMMENT"].front());
    oneValueSet.append("COMMENT");
  } else
    setComment(String());

  if(properties.contains("GENRE")) {
    setGenre(properties["GENRE"].front());
    oneValueSet.append("GENRE");
  } else
    setGenre(String());

  if(properties.contains("DATE")) {
    bool ok;
    int date = properties["DATE"].front().toInt(&ok);
    if(ok) {
      setYear(date);
      oneValueSet.append("DATE");
    } else
      setYear(0);
  }
  else
    setYear(0);

  if(properties.contains("TRACKNUMBER")) {
    bool ok;
    int track = properties["TRACKNUMBER"].front().toInt(&ok);
    if(ok) {
      setTrack(track);
      oneValueSet.append("TRACKNUMBER");
    } else
      setTrack(0);
  }
  else
    setTrack(0);

  if(properties.contains("DISCNUMBER")) {
    bool ok;
    int disc = properties["DISCNUMBER"].front().toInt(&ok);
    if(ok) {
      setDisc(disc);
      oneValueSet.append("DISCNUMBER");
    } else
      setDisc(0);
  }
  else
    setDisc(0);

  // for each tag that has been set above, remove the first entry in the corresponding
  // value list. The others will be returned as unsupported by this format.
  for(StringList::ConstIterator it = oneValueSet.begin(); it != oneValueSet.end(); ++it) {
    if(properties[*it].size() == 1)
      properties.erase(*it);
    else
      properties[*it].erase( properties[*it].begin() );
  }
  return properties;
}

void Tag::duplicate(const Tag *source, Tag *target, bool overwrite) // static
{
  if(overwrite) {
    target->setTitle(source->title());
    target->setAlbumArtist(source->albumartist());
    target->setArtist(source->artist());
    target->setComposer(source->composer());
    target->setAlbum(source->album());
    target->setUnsyncedlyrics(source->unsyncedlyrics());
    target->setComment(source->comment());
    target->setGenre(source->genre());
    target->setYear(source->year());
    target->setTrack(source->track());
    target->setDisc(source->disc());
  }
  else {
    if(target->title().isEmpty())
      target->setTitle(source->title());
    if(target->albumartist().isEmpty())
      target->setAlbumArtist(source->albumartist());
    if(target->artist().isEmpty())
      target->setArtist(source->artist());
    if(target->composer().isEmpty())
      target->setComposer(source->composer());
    if(target->album().isEmpty())
      target->setAlbum(source->album());
    if(target->unsyncedlyrics().isEmpty())
      target->setUnsyncedlyrics(source->unsyncedlyrics());
    if(target->comment().isEmpty())
      target->setComment(source->comment());
    if(target->genre().isEmpty())
      target->setGenre(source->genre());
    if(target->year() <= 0)
      target->setYear(source->year());
    if(target->track() <= 0)
      target->setTrack(source->track());
    if(target->disc() <= 0)
      target->setDisc(source->disc());
  }
}
