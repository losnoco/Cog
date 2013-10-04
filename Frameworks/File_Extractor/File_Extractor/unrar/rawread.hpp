#ifndef _RAR_RAWREAD_
#define _RAR_RAWREAD_

class RawRead
{
private:
	Array<byte> Data;
	File *SrcFile;
	long DataSize;
	long ReadPos;
	friend class Archive;
public:
	RawRead(File *SrcFile);
	void Reset();
	void Read(long Size);
	void Get(byte &Field);
	void Get(ushort &Field);
	void Get(uint &Field);
	void Get(byte *Field,int Size);
	uint GetCRC(bool ProcessedOnly);
	long Size() {return DataSize;}
    long PaddedSize() {return Data.Size()-DataSize;}
};

#endif
