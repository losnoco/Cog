#ifndef MP4ITUNESTAG_H
#define MP4ITUNESTAG_H

#include "taglib.h"
#include "tstring.h"
#include "tag.h"

namespace TagLib
{
  namespace MP4
  {
    class File;

    class Tag : public TagLib::Tag
    {
    public:
      /*!
       * Constructs an empty MP4 iTunes tag.
       */
      Tag( );
  
      /*!
       * Destroys this Tag instance.
       */
      virtual ~Tag();
  
      // Reimplementations.
  
      virtual String title() const;
      virtual String artist() const;
      virtual String album() const;
      virtual String comment() const;
      virtual String genre() const;
      virtual uint year() const;
      virtual uint track() const;
  
      virtual void setTitle(const String &s);
      virtual void setArtist(const String &s);
      virtual void setAlbum(const String &s);
      virtual void setComment(const String &s);
      virtual void setGenre(const String &s);
      virtual void setYear(uint i);
      virtual void setTrack(uint i);

      // MP4 specific fields

      String     grouping() const;
      String     composer() const;
      uint       numTracks() const;
      uint       disk() const;
      uint       numDisks() const;
      uint       bpm() const;
      ByteVector cover() const;
      
      void setGrouping(const String &s);
      void setComposer(const String &s);
      void setNumTracks(const uint i);
      void setDisk(const uint i);
      void setNumDisks(const uint i);
      void setBpm(const uint i);
      void setCover( const ByteVector& cover );
  
      virtual bool isEmpty() const;
  
    private:
      Tag(const Tag &);
      Tag &operator=(const Tag &);
  
      class TagPrivate;
      TagPrivate *d;
    };
  } // namespace MP4

} // namespace TagLib

#endif // MP4ITUNESTAG_H
