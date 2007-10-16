#include "mp4fourcc.h"

using namespace TagLib;

MP4::Fourcc::Fourcc()
{
  m_fourcc = 0U;
}

MP4::Fourcc::Fourcc( TagLib::String fourcc )
{
  m_fourcc = 0U;

  if( fourcc.size() >= 4 )
    m_fourcc = static_cast<unsigned char>(fourcc[0]) << 24 |
               static_cast<unsigned char>(fourcc[1]) << 16 |
               static_cast<unsigned char>(fourcc[2]) <<  8 |
               static_cast<unsigned char>(fourcc[3]);
}

MP4::Fourcc::~Fourcc()
{}

TagLib::String MP4::Fourcc::toString() const
{
  TagLib::String fourcc;
  fourcc.append(static_cast<char>(m_fourcc >> 24 & 0xFF));
  fourcc.append(static_cast<char>(m_fourcc >> 16 & 0xFF));
  fourcc.append(static_cast<char>(m_fourcc >>  8 & 0xFF));
  fourcc.append(static_cast<char>(m_fourcc       & 0xFF));

  return fourcc;
}

MP4::Fourcc::operator unsigned int() const
{
  return m_fourcc;
}

bool MP4::Fourcc::operator == (unsigned int fourccB ) const
{
  return (m_fourcc==fourccB);
}

bool MP4::Fourcc::operator != (unsigned int fourccB ) const
{
  return (m_fourcc!=fourccB);
}

MP4::Fourcc& MP4::Fourcc::operator = (unsigned int fourcc )
{
  m_fourcc = fourcc;
  return *this;
}

MP4::Fourcc& MP4::Fourcc::operator = (char fourcc[4])
{
  m_fourcc = static_cast<unsigned char>(fourcc[0]) << 24 |
             static_cast<unsigned char>(fourcc[1]) << 16 |
             static_cast<unsigned char>(fourcc[2]) <<  8 |
             static_cast<unsigned char>(fourcc[3]);
  return *this;
}
