/*
 *  SourceCIO.h
 *  MonkeysAudio
 *
 *  Created by Zaphod Beeblebrox on 3/4/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include <MAC/IO.h>

#include "Plugin.h"

class SourceIO : public CIO
{
public:
	//construction / destruction
	SourceIO(id<CogSource> s);
	~SourceIO();
	
	// open / close
	int Open(const wchar_t * pName);
	int Close();
    
    // read / write
    int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead);
    int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten);
    
    // seek
    int Seek(int nDistance, unsigned int nMoveMode);
    
    // creation / destruction
    int Create(const wchar_t * pName);
    int Delete();
	
    // other functions
    int SetEOF();
	
    // attributes
    int GetPosition();
    int GetSize();
	int GetName(wchar_t * pBuffer);
protected:
	id<CogSource> source;
};
