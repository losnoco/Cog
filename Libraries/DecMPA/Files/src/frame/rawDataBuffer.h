/*
  stores simple buffer information. does not allocate anything
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */



#ifndef __RAWDATABUFFER_H
#define __RAWDATABUFFER_H


class RawDataBuffer {

  int _size;
  unsigned char* _ptr;
  int _pos;

 public:
  RawDataBuffer(unsigned char* ptr,int size) { set(ptr,size,0); }
  ~RawDataBuffer();



  unsigned char* ptr()                { return _ptr;        }
  unsigned char* current()            { return _ptr+_pos;   }
  int size()                          { return _size;       }
  int pos()                           { return _pos;        }
  int untilend()                      { return _size-_pos;  }
  int eof()                           { return _pos>=_size; }

  void inc()                          { this->_pos++;       }
  void inc(int val)                   { this->_pos+=val;    }
  void setpos(int val)                { this->_pos=val;     }
  void setptr(unsigned char* ptr)     { this->_ptr=ptr;     }
  void setsize(int size)              { this->_size=size;   }

  void set(unsigned char* ptr,
	   int size,int pos)          { setpos(pos);setptr(ptr);setsize(size);}
  
};
#endif
