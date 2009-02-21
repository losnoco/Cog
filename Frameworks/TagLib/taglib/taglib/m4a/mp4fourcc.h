#ifndef MP4FOURCC_H
#define MP4FOURCC_H

#include "tstring.h"

namespace TagLib
{
  namespace MP4
  {
    /*! union for easy fourcc / type handling */
    class Fourcc
    {
    public:
      //! std constructor
      Fourcc();
      //! string constructor
      Fourcc(TagLib::String fourcc);

      //! destructor
      ~Fourcc();

      //! function to get a string version of the fourcc
      TagLib::String toString() const;
      //! cast operator to unsigned int
      operator unsigned int() const;
      //! comparison operator
      bool operator == (unsigned int fourccB ) const;
      //! comparison operator
      bool operator != (unsigned int fourccB ) const;
      //! assigment operator for unsigned int
      Fourcc& operator = (unsigned int fourcc );
      //! assigment operator for character string
      Fourcc& operator = (char fourcc[4]);

    private:
      uint m_fourcc;     /*!< integer code of the fourcc */
    };

  } // namespace MP4
} // namespace TagLib

#endif // MP4FOURCC_H
