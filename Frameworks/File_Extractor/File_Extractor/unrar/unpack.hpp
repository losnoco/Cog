#ifndef _RAR_UNPACK_
#define _RAR_UNPACK_

enum BLOCK_TYPES {BLOCK_LZ,BLOCK_PPM};

struct Decode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[2];
};

struct LitDecode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[NC];
};

struct DistDecode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[DC];
};

struct LowDistDecode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[LDC];
};

struct RepDecode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[RC];
};

struct BitDecode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[BC];
};

struct UnpackFilter
	: Rar_Allocator
{
	unsigned long BlockStart;
	unsigned long BlockLength;
	unsigned long ExecCount;
	bool NextWindow;

	// position of parent filter in Filters array used as prototype for filter
	// in PrgStack array. Not defined for filters in Filters array.
	unsigned long ParentFilter;

	VM_PreparedProgram Prg;
	UnpackFilter( Rar_Error_Handler* eh ) : Prg( eh ) { }
};

/***************************** Unpack v 2.0 *********************************/
struct MultDecode
{
	uint MaxNum;
	uint DecodeLen[16];
	uint DecodePos[16];
	uint DecodeNum[MC20];
};

struct AudioVariables
{
	int K1,K2,K3,K4,K5;
	int D1,D2,D3,D4;
	int LastDelta;
	uint Dif[11];
	uint ByteCount;
	long LastChar;
};
/***************************** Unpack v 2.0 *********************************/


// public so operator new/delete will be accessible, argh 
class Unpack:public BitInput
{
private:
    friend class Pack;

	void Unpack29(bool Solid);
	bool UnpReadBuf();
	void UnpWriteBuf();
	void ExecuteCode(VM_PreparedProgram *Prg);
	void UnpWriteArea(uint StartPtr,uint EndPtr);
	void UnpWriteData(byte *Data,long Size);
	bool ReadTables();
	void MakeDecodeTables(unsigned char *LenTab,struct Decode *Dec,long Size);
	long DecodeNumber(struct Decode *Dec);
	void CopyString();
	inline void InsertOldDist(uint Distance);
	inline void InsertLastMatch(uint Length,uint Distance);
	void UnpInitData(int Solid);
	void CopyString(uint Length,uint Distance);
	bool ReadEndOfBlock();
	bool ReadVMCode();
	bool ReadVMCodePPM();
	bool AddVMCode(uint FirstByte,byte *Code,long CodeSize);
	void InitFilters();

	ComprDataIO *UnpIO;
	ModelPPM PPM;
	int PPMEscChar;

	Array<byte> VMCode; // here to avoid leaks
	BitInput Inp; // here to avoid leaks

	RarVM VM;
	
	UnpackFilter* LastStackFilter; // avoids leak for stack-based filter
	
    /* Filters code, one entry per filter */
	Array<UnpackFilter*> Filters;

    /* Filters stack, several entrances of same filter are possible */
	Array<UnpackFilter*> PrgStack;

    /* lengths of preceding blocks, one length per filter. Used to reduce
       size required to write block length if lengths are repeating */
	Array<long> OldFilterLengths;

	long LastFilter;

	bool TablesRead;
	struct LitDecode LD;
	struct DistDecode DD;
	struct LowDistDecode LDD;
	struct RepDecode RD;
	struct BitDecode BD;

	uint OldDist[4],OldDistPtr;
	uint LastDist,LastLength;

	uint UnpPtr,WrPtr;

	long ReadTop;
	long ReadBorder;

	unsigned char UnpOldTable[HUFF_TABLE_SIZE];

	int UnpBlockType;

	byte *Window;
	bool ExternalWindow;


	Int64 DestUnpSize;

	enum { Suspended = false }; // original source could never set to true
	bool UnpAllBuf;
	bool UnpSomeRead;
	Int64 WrittenFileSize;
	bool FileExtracted;

	long PrevLowDist,LowDistRepCount;

	/***************************** Unpack v 1.5 *********************************/
	void Unpack15(bool Solid);
	void ShortLZ();
	void LongLZ();
	void HuffDecode();
	void GetFlagsBuf();
	void OldUnpInitData(int Solid);
	void InitHuff();
	void CorrHuff(uint *CharSet,uint *NumToPlace);
	void OldCopyString(uint Distance,uint Length);
	uint DecodeNum(long Num,uint StartPos,
	  const uint *DecTab,const uint *PosTab);
	void OldUnpWriteBuf();

	uint ChSet[256],ChSetA[256],ChSetB[256],ChSetC[256];
	uint Place[256],PlaceA[256],PlaceB[256],PlaceC[256];
	uint NToPl[256],NToPlB[256],NToPlC[256];
	uint FlagBuf,AvrPlc,AvrPlcB,AvrLn1,AvrLn2,AvrLn3;
	long Buf60,NumHuf,StMode,LCount,FlagsCnt;
	uint Nhfb,Nlzb,MaxDist3;
	/***************************** Unpack v 1.5 *********************************/

	/***************************** Unpack v 2.0 *********************************/
	void Unpack20(bool Solid);
	struct MultDecode MD[4];
	unsigned char UnpOldTable20[MC20*4];
	int UnpAudioBlock,UnpChannels,UnpCurChannel,UnpChannelDelta;
	void CopyString20(uint Length,uint Distance);
	bool ReadTables20();
	void UnpInitData20(int Solid);
	void ReadLastTables();
	byte DecodeAudio(long Delta);
	struct AudioVariables AudV[4];
	/***************************** Unpack v 2.0 *********************************/

public:
	Rar_Error_Handler& ErrHandler;
	byte const* window_wrptr() const { return &Window [WrPtr & MAXWINMASK]; }
	
	static void init_tables();
	Unpack(ComprDataIO *DataIO);
	~Unpack();
	void Init(byte *Window=NULL);
	void DoUnpack(int Method,bool Solid);
	void SetDestSize(Int64 DestSize) {DestUnpSize=DestSize;FileExtracted=false;}

	unsigned int GetChar()
	{
	  if (InAddr>BitInput::MAX_SIZE-30)
	    UnpReadBuf();
	  return(InBuf[InAddr++]);
	}
};

#endif
