/***************************************************************************
    copyright            : (C) 2006 by Lukáš Lalinský
    email                : lalinsky@gmail.com

    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
                           (original Vorbis implementation)
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include <tstring.h>
#include <tdebug.h>

#include <oggpageheader.h>

#include "opusproperties.h"
#include "opusfile.h"

using namespace TagLib;
using namespace TagLib::Ogg;

class Opus::Properties::PropertiesPrivate
{
public:
  PropertiesPrivate(File *f, ReadStyle s) :
    file(f),
    style(s),
    length(0),
    inputSampleRate(0),
    bitrate(0),
    preSkip(0),
    channels(0),
    opusVersion(0),
    outputGain(0) {}

  File *file;
  ReadStyle style;
  int length;
  int inputSampleRate;
  int bitrate;
  int preSkip;
  int channels;
  int opusVersion;
  int outputGain;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Opus::Properties::Properties(File *file, ReadStyle style) : AudioProperties(style)
{
  d = new PropertiesPrivate(file, style);
  read();
}

Opus::Properties::~Properties()
{
  delete d;
}

int Opus::Properties::length() const
{
  return d->length;
}

int Opus::Properties::bitrate() const
{
  return int(float(d->bitrate) / float(1000) + 0.5);
}

int Opus::Properties::sampleRate() const
{
  return 48000;
}

int Opus::Properties::channels() const
{
  return d->channels;
}

int Opus::Properties::opusVersion() const
{
  return d->opusVersion;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Opus::Properties::read()
{
  // Get the identification header from the Ogg implementation.

  ByteVector data = d->file->packet(0);

  int pos = 8;

  // opus_version_id;       /**< Version for Opus (for checking compatibility) */
  d->opusVersion = data.mid(pos, 1).toUInt(false);
  pos += 1;

  // nb_channels;            /**< Number of channels encoded */
  d->channels = data.mid(pos, 1).toUInt(false);
  pos += 1;

  // pre_skip
  d->preSkip = data.mid(pos, 2).toUInt(false);
  pos += 2;

  // rate;                   /**< Sampling rate used */
  d->inputSampleRate = data.mid(pos, 4).toUInt(false);
  pos += 4;

  // output_gain;
  d->outputGain = data.mid(pos, 2).toUInt(false);
  pos += 2;
    
  // frames_per_packet;      /**< Number of frames stored per Ogg packet */
  // unsigned int framesPerPacket = data.mid(pos, 4).toUInt(false);

  const Ogg::PageHeader *first = d->file->firstPageHeader();
  const Ogg::PageHeader *last = d->file->lastPageHeader();

  if(first && last) {
    long long start = first->absoluteGranularPosition();
    long long end = last->absoluteGranularPosition();

    if(start >= 0 && end >= 0)
      d->length = (int) ((end - start) / (long long) 48000);
    else
      debug("Opus::Properties::read() -- Either the PCM values for the start or "
            "end of this file was incorrect or the sample rate is zero.");
    
    ByteVector comments = d->file->packet(1);
    d->bitrate = ( d->file->length() - comments.size() ) * 8 / d->length;
  }
  else
    debug("Opus::Properties::read() -- Could not find valid first and last Ogg pages.");
}
