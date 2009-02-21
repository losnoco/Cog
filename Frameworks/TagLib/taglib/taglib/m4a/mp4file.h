/***************************************************************************
    copyright            : (C) 2002, 2003 by Jochen Issing
    email                : jochen.issing@isign-softart.de
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
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
 ***************************************************************************/

#ifndef TAGLIB_MP4FILE_H
#define TAGLIB_MP4FILE_H

#include <tfile.h>
#include <audioproperties.h>

#include "mp4fourcc.h"

namespace TagLib {

  typedef unsigned long long ulonglong;

  class Tag;

  namespace MP4
  {
    class Mp4TagsProxy;
    class Mp4PropsProxy;

    //! An implementation of TagLib::File with mp4 itunes specific methods

    /*!
     * This implements and provides an interface for mp4 itunes files to the
     * TagLib::Tag and TagLib::AudioProperties interfaces by way of implementing
     * the abstract TagLib::File API as well as providing some additional
     * information specific to mp4 itunes files. (TODO)
     */

    class File : public TagLib::File
    {
    public:
      /*!
       * Contructs an mp4 itunes file object without reading a file.  Allows object
       * fields to be set up before reading.
       */
      File();

      /*!
       * Contructs an mp4 itunes file from \a file.  If \a readProperties is true the
       * file's audio properties will also be read using \a propertiesStyle.  If
       * false, \a propertiesStyle is ignored.
       */
      File(FileName file, bool readProperties = true,
           TagLib::AudioProperties::ReadStyle propertiesStyle = TagLib::AudioProperties::Average);

      /*!
       * Destroys this instance of the File.
       */
      virtual ~File();

      /*!
       * Returns the Tag for this file.  This will be an APE tag, an ID3v1 tag
       * or a combination of the two.
       */
      virtual TagLib::Tag *tag() const;

      /*!
       * Returns the mp4 itunes::Properties for this file.  If no audio properties
       * were read then this will return a null pointer.
       */
      virtual AudioProperties *audioProperties() const;

      /*!
       * Reads from mp4 itunes file.  If \a readProperties is true the file's
       * audio properties will also be read using \a propertiesStyle.  If false,
       * \a propertiesStyle is ignored.
       */
      void read(bool readProperties = true,
                TagLib::AudioProperties::ReadStyle propertiesStyle = TagLib::AudioProperties::Average);

      /*!
       * Saves the file.
       */
      virtual bool save();

      /*!
       * This will remove all tags.
       *
       * \note This will also invalidate pointers to the tags
       * as their memory will be freed.
       * \note In order to make the removal permanent save() still needs to be called
       */
      void remove();

      /*!
       * Helper function for parsing the MP4 file - reads the size and type of the next box.
       * Returns true if read succeeded - not at EOF
       */
      bool readSizeAndType( TagLib::uint& size, MP4::Fourcc& fourcc );

      /*!
       * Helper function to read the length of an descriptor in systems manner
       */
      TagLib::uint readSystemsLen();

      /*!
       * Helper function for reading an unsigned int out of the file (big endian method)
       */
      bool readInt( TagLib::uint& toRead );

      /*!
       * Helper function for reading an unsigned short out of the file (big endian method)
       */
      bool readShort( TagLib::uint& toRead );
      
      /*!
       * Helper function for reading an unsigned long long (64bit) out of the file (big endian method)
       */
      bool readLongLong( TagLib::ulonglong& toRead );

      /*!
       *  Helper function to read a fourcc code
       */
      bool readFourcc( TagLib::MP4::Fourcc& fourcc );

      /*!
       * Function to get the tags proxy for registration of the tags boxes.
       * The proxy provides direct access to the data boxes of the certain tags - normally
       * covered by several levels of subboxes
       */
      Mp4TagsProxy* tagProxy() const;

      /*!
       * Function to get the properties proxy for registration of the properties boxes.
       * The proxy provides direct access to the needed boxes describing audio properties.
       */
      Mp4PropsProxy* propProxy() const;

    private:
      File(const File &);
      File &operator=(const File &);

      class FilePrivate;
      FilePrivate *d;
    };

  } // namespace MP4

} // namespace TagLib

#endif // TAGLIB_MP4FILE_H
