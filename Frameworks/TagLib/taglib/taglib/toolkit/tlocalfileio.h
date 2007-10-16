/***************************************************************************
    copyright            : (C) 2002, 2003 by Scott Wheeler
    email                : wheeler@kde.org
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

#ifndef TAGLIB_LOCALFILE_H
#define TAGLIB_LOCALFILE_H

#include "taglib.h"
#include "tbytevector.h"
#include "tfileio.h"

namespace TagLib {

  //! A file I/O class supporting local files

  /*!
   * This class is a basic file I/O class providing support for accessing local
   * files.
   */

  class LocalFileIO : public TagLib::FileIO
  {
  public:

    /*!
     * Construct a File I/O object and opens the \a file.  \a file should be a
     * be a C-string in the local file system encoding.
     */
    LocalFileIO(const char *file);

    /*!
     * Destroys this File instance.
     */
    virtual ~LocalFileIO();

    /*!
     * Returns the file name in the local file system encoding.
     */
    virtual const char *name() const;

    /*!
     * Reads a block of size \a length at the current get pointer.
     */
    virtual ByteVector readBlock(ulong length);

    /*!
     * Attempts to write the block \a data at the current get pointer.  If the
     * file is currently only opened read only -- i.e. readOnly() returns true --
     * this attempts to reopen the file in read/write mode.
     *
     * \note This should be used instead of using the streaming output operator
     * for a ByteVector.  And even this function is significantly slower than
     * doing output with a char[].
     */
    virtual void writeBlock(const ByteVector &data);

    /*!
     * Insert \a data at position \a start in the file overwriting \a replace
     * bytes of the original content.
     *
     * \note This method is slow since it requires rewriting all of the file
     * after the insertion point.
     */
    virtual void insert(const ByteVector &data, ulong start = 0, ulong replace = 0);

    /*!
     * Removes a block of the file starting a \a start and continuing for
     * \a length bytes.
     *
     * \note This method is slow since it involves rewriting all of the file
     * after the removed portion.
     */
    virtual void removeBlock(ulong start = 0, ulong length = 0);

    /*!
     * Returns true if the file is read only (or if the file can not be opened).
     */
    virtual bool readOnly() const;

    /*!
     * Since the file can currently only be opened as an argument to the
     * constructor (sort-of by design), this returns if that open succeeded.
     */
    virtual bool isOpen() const;

    /*!
     * Move the I/O pointer to \a offset in the file from position \a p.  This
     * defaults to seeking from the beginning of the file.
     *
     * \see Position
     */
    virtual int seek(long offset, Position p = Beginning);

    /*!
     * Reset the end-of-file and error flags on the file.
     */
    virtual void clear();

    /*!
     * Returns the current offset withing the file.
     */
    virtual long tell() const;

    /*!
     * Returns the length of the file.
     */
    virtual long length();

    /*!
     * Returns true if \a file can be opened for reading.  If the file does not
     * exist, this will return false.
     */
    static bool isReadable(const char *file);

    /*!
     * Returns true if \a file can be opened for writing.
     */
    static bool isWritable(const char *name);

  protected:
    /*!
     * Truncates the file to a \a length.
     */
    virtual void truncate(long length);

    /*!
     * Returns the buffer size that is used for internal buffering.
     */
    static uint bufferSize();

  private:
    LocalFileIO(const LocalFileIO &);
    LocalFileIO &operator=(const LocalFileIO &);

    class LocalFileIOPrivate;
    LocalFileIOPrivate *d;
  };

}

#endif
