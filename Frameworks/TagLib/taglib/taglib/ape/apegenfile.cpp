/***************************************************************************
    copyright            : (C) 2010 by Alex Novichkov
    email                : novichko@atnet.ru

    copyright            : (C) 2006 by Lukáš Lalinský
    email                : lalinsky@gmail.com
                           (original WavPack implementation)

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
#include <taglib/mpeg/id3v1/id3v1tag.h>
#include <taglib/mpeg/id3v2/id3v2header.h>
#include <taglib/toolkit/tpropertymap.h>
#include <taglib/tagutils.h>

#include <taglib/ape/apegenfile.h>
#include <taglib/ape/apetag.h>
#include <taglib/ape/apefooter.h>

using namespace TagLib;

namespace
{
  enum { ApeGenAPEIndex = 0 };
}

class APEGen::File::FilePrivate
{
public:
  FilePrivate() :
    APELocation(-1),
    APESize(0) {}

  ~FilePrivate()
  {
  }

  long APELocation;
  long APESize;

  TagUnion tag;
};

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool APEGen::File::isSupported(IOStream *stream)
{
    // Generic file support for anything with APE tags
    const ByteVector buffer = Utils::readHeader(stream, bufferSize(), true);
    return (buffer.find("APETAGEX") >= 0);
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

APEGen::File::File(FileName file, bool readProperties, Properties::ReadStyle) :
  TagLib::File(file),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

APEGen::File::File(IOStream *stream, bool readProperties, Properties::ReadStyle) :
  TagLib::File(stream),
  d(new FilePrivate())
{
  if(isOpen())
    read(readProperties);
}

APEGen::File::~File()
{
  delete d;
}

TagLib::Tag *APEGen::File::tag() const
{
  return &d->tag;
}

PropertyMap APEGen::File::properties() const
{
  return d->tag.properties();
}

void APEGen::File::removeUnsupportedProperties(const StringList &properties)
{
  d->tag.removeUnsupportedProperties(properties);
}

PropertyMap APEGen::File::setProperties(const PropertyMap &properties)
{
  return APETag(true)->setProperties(properties);
}

APEGen::Properties *APEGen::File::audioProperties() const
{
  return NULL;
}

bool APEGen::File::save()
{
  if(readOnly()) {
    debug("APEGen::File::save() -- File is read only.");
    return false;
  }

  // Update APE tag

  if(APETag() && !APETag()->isEmpty()) {

    // APE tag is not empty. Update the old one or create a new one.

    if(d->APELocation < 0) {
      d->APELocation = length();
    }

    const ByteVector data = APETag()->render();
    insert(data, d->APELocation, d->APESize);

    d->APESize = data.size();
  }
  else {

    // APE tag is empty. Remove the old one.

    if(d->APELocation >= 0) {
      removeBlock(d->APELocation, d->APESize);

      d->APELocation = -1;
      d->APESize = 0;
    }
  }

  return true;
}

APE::Tag *APEGen::File::APETag(bool create)
{
  return d->tag.access<APE::Tag>(ApeGenAPEIndex, create);
}

void APEGen::File::strip(int tags)
{
  if(tags & APE)
    d->tag.set(ApeGenAPEIndex, 0);

  APETag(true);
}

bool APEGen::File::hasAPETag() const
{
  return (d->APELocation >= 0);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void APEGen::File::read(bool readProperties)
{
  // Look for an APE tag

  d->APELocation = Utils::findAPE(this, -1);

  if(d->APELocation >= 0) {
    d->tag.set(ApeGenAPEIndex, new APE::Tag(this, d->APELocation));
    d->APESize = APETag()->footer()->completeTagSize();
    d->APELocation = d->APELocation + APE::Footer::size() - d->APESize;
  }

  APETag(true);
}
