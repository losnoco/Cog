/***************************************************************************
    copyright            : (C) 2006 by Lukáš Lalinský
    email                : lalinsky@gmail.com

    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.org
                           (original MPC implementation)
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

#include <taglib/toolkit/tbytevector.h>
#include <taglib/toolkit/tstring.h>
#include <taglib/toolkit/tdebug.h>
#include <taglib/tagunion.h>
#include <taglib/toolkit/tstringlist.h>
#include <taglib/toolkit/tpropertymap.h>
#include <taglib/tagutils.h>

#include <taglib/trueaudio/trueaudiofile.h>
#include <taglib/mpeg/id3v1/id3v1tag.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/id3v2header.h>
#include <taglib/ape/apefooter.h>
#include <taglib/ape/apetag.h>

using namespace TagLib;

namespace
{
  enum { TrueAudioID3v2Index = 0, TrueAudioAPEIndex = 1, TrueAudioID3v1Index = 2 };
}

class TrueAudio::File::FilePrivate
{
public:
  FilePrivate(const ID3v2::FrameFactory *frameFactory = ID3v2::FrameFactory::instance()) :
    ID3v2FrameFactory(frameFactory),
    ID3v2Location(-1),
    ID3v2OriginalSize(0),
    ID3v1Location(-1),
    APELocation(-1),
    APEOriginalSize(0),
    properties(0) {}

  ~FilePrivate()
  {
    delete properties;
  }

  const ID3v2::FrameFactory *ID3v2FrameFactory;
  long ID3v2Location;
  long ID3v2OriginalSize;

  long APELocation;
  long APEOriginalSize;

  long ID3v1Location;

  TagUnion tag;

  Properties *properties;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool TrueAudio::File::isSupported(IOStream *stream)
{
  // A TrueAudio file has to start with "TTA". An ID3v2 tag may precede.

  const ByteVector id = Utils::readHeader(stream, 3, true);
  return (id == "TTA");
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TrueAudio::File::File(FileName file, bool readProperties, Properties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

TrueAudio::File::File(FileName file, ID3v2::FrameFactory *frameFactory,
                      bool readProperties, Properties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate(frameFactory))
{
  if(isOpen())
    read(readProperties);
}

TrueAudio::File::File(IOStream *stream, bool readProperties, Properties::ReadStyle) :
  TagLib::File(stream),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

TrueAudio::File::File(IOStream *stream, ID3v2::FrameFactory *frameFactory,
                      bool readProperties, Properties::ReadStyle) :
  TagLib::File(stream),
  d(new FilePrivate(frameFactory))
{
  if(isOpen())
    read(readProperties);
}

TrueAudio::File::~File()
{
  delete d;
}

TagLib::Tag *TrueAudio::File::tag() const
{
  return &d->tag;
}

PropertyMap TrueAudio::File::properties() const
{
  return d->tag.properties();
}

void TrueAudio::File::removeUnsupportedProperties(const StringList &unsupported)
{
  d->tag.removeUnsupportedProperties(unsupported);
}

PropertyMap TrueAudio::File::setProperties(const PropertyMap &properties)
{
  if(ID3v1Tag())
    ID3v1Tag()->setProperties(properties);

  if(ID3v2Tag())
    ID3v2Tag()->setProperties(properties);

  return APETag(true)->setProperties(properties);
}

TrueAudio::Properties *TrueAudio::File::audioProperties() const
{
  return d->properties;
}

void TrueAudio::File::setID3v2FrameFactory(const ID3v2::FrameFactory *factory)
{
  d->ID3v2FrameFactory = factory;
}

bool TrueAudio::File::save()
{
  if(readOnly()) {
    debug("TrueAudio::File::save() -- File is read only.");
    return false;
  }

  // Update ID3v2 tag

  if(ID3v2Tag() && !ID3v2Tag()->isEmpty()) {

    // ID3v2 tag is not empty. Update the old one or create a new one.

    if(d->ID3v2Location < 0)
      d->ID3v2Location = 0;

    const ByteVector data = ID3v2Tag()->render();
    insert(data, d->ID3v2Location, d->ID3v2OriginalSize);

    if(d->APELocation >= 0)
      d->APELocation += (static_cast<long>(data.size()) - d->ID3v2OriginalSize);

    if(d->ID3v1Location >= 0)
      d->ID3v1Location += (static_cast<long>(data.size()) - d->ID3v2OriginalSize);

    d->ID3v2OriginalSize = data.size();
  }
  else {

    // ID3v2 tag is empty. Remove the old one.

    if(d->ID3v2Location >= 0) {
      removeBlock(d->ID3v2Location, d->ID3v2OriginalSize);

      if(d->APELocation >= 0)
        d->APELocation -= d->ID3v2OriginalSize;

      if(d->ID3v1Location >= 0)
        d->ID3v1Location -= d->ID3v2OriginalSize;

      d->ID3v2Location = -1;
      d->ID3v2OriginalSize = 0;
    }
  }

  // Update ID3v1 tag

  if(ID3v1Tag() && !ID3v1Tag()->isEmpty()) {

    // ID3v1 tag is not empty. Update the old one or create a new one.

    if(d->ID3v1Location >= 0) {
      seek(d->ID3v1Location);
    }
    else {
      seek(0, End);
      d->ID3v1Location = tell();
    }

    writeBlock(ID3v1Tag()->render());
  }
  else {

    // ID3v1 tag is empty. Remove the old one.

    if(d->ID3v1Location >= 0) {
      truncate(d->ID3v1Location);
      d->ID3v1Location = -1;
    }
  }

  if(APETag() && !APETag()->isEmpty()) {
    // APE tag is not empty. Update the old one or create a new one.
    if(d->APELocation < 0) {
      if(d->ID3v1Location >= 0)
        d->APELocation = d->ID3v1Location;
      else
        d->APELocation = length();
    }

    const ByteVector data = APETag()->render();
    insert(data, d->APELocation, d->APEOriginalSize);

    if(d->ID3v1Location >= 0)
        d->ID3v1Location += (static_cast<long>(data.size()) - d->APEOriginalSize);

    d->APEOriginalSize = data.size();
  }
  else {
    // APE tag is empty. Remove the old one.

    if(d->APELocation >= 0) {
      removeBlock(d->APELocation, d->APEOriginalSize);

      if (d->ID3v1Location >= 0) {
        d->ID3v1Location -= d->APEOriginalSize;
      }
    }
  }

  return true;
}

ID3v1::Tag *TrueAudio::File::ID3v1Tag(bool create)
{
  return d->tag.access<ID3v1::Tag>(TrueAudioID3v1Index, create);
}

ID3v2::Tag *TrueAudio::File::ID3v2Tag(bool create)
{
  return d->tag.access<ID3v2::Tag>(TrueAudioID3v2Index, create);
}

APE::Tag *TrueAudio::File::APETag(bool create)
{
  return d->tag.access<APE::Tag>(TrueAudioAPEIndex, create);
}

void TrueAudio::File::strip(int tags)
{
  if(tags & ID3v1)
    d->tag.set(TrueAudioID3v1Index, 0);

  if(tags & ID3v2)
    d->tag.set(TrueAudioID3v2Index, 0);

  if(tags & APE)
    d->tag.set(TrueAudioAPEIndex, 0);

  if(!ID3v1Tag())
    APETag(true);
}

bool TrueAudio::File::hasID3v1Tag() const
{
  return (d->ID3v1Location >= 0);
}

bool TrueAudio::File::hasID3v2Tag() const
{
  return (d->ID3v2Location >= 0);
}

bool TrueAudio::File::hasAPETag() const
{
  return (d->APELocation >= 0);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TrueAudio::File::read(bool readProperties)
{
  // Look for an ID3v2 tag

  d->ID3v2Location = Utils::findID3v2(this);

  if(d->ID3v2Location >= 0) {
    d->tag.set(TrueAudioID3v2Index, new ID3v2::Tag(this, d->ID3v2Location, d->ID3v2FrameFactory));
    d->ID3v2OriginalSize = ID3v2Tag()->header()->completeTagSize();
  }

  // Look for an ID3v1 tag

  d->ID3v1Location = Utils::findID3v1(this);

  if(d->ID3v1Location >= 0)
    d->tag.set(TrueAudioID3v1Index, new ID3v1::Tag(this, d->ID3v1Location));

  if(d->ID3v1Location < 0)
    APETag(true);
	
  // Look for an APE tag

  d->APELocation = Utils::findAPE(this, d->ID3v1Location);

  if(d->APELocation >= 0) {
    d->tag.set(TrueAudioAPEIndex, new APE::Tag(this, d->APELocation));
    d->APEOriginalSize = APETag()->footer()->completeTagSize();
    d->APELocation = d->APELocation + APE::Footer::size() - d->APEOriginalSize;
  }

  // Look for TrueAudio metadata

  if(readProperties) {

    long streamLength;

    if(d->ID3v1Location >= 0)
      streamLength = d->ID3v1Location;
    else
      streamLength = length();

    if(d->ID3v2Location >= 0) {
      seek(d->ID3v2Location + d->ID3v2OriginalSize);
      streamLength -= (d->ID3v2Location + d->ID3v2OriginalSize);
    }
    else {
      seek(0);
    }

    d->properties = new Properties(readBlock(TrueAudio::HeaderSize), streamLength);
  }
}
