/*
 * ContainerMMCMP.cpp
 * ------------------
 * Purpose: Handling of MMCMP compressed modules
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "../common/FileReader.h"
#include "Container.h"
#include "Sndfile.h"
#include "BitReader.h"


OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_ALL_LOGGING
#define MMCMP_LOG
#endif


struct MMCMPFILEHEADER
{
	char     id[8];	// "ziRCONia"
	uint16le hdrsize;
};

MPT_BINARY_STRUCT(MMCMPFILEHEADER, 10)

struct MMCMPHEADER
{
	uint16le version;
	uint16le nblocks;
	uint32le filesize;
	uint32le blktable;
	uint8le  glb_comp;
	uint8le  fmt_comp;
};

MPT_BINARY_STRUCT(MMCMPHEADER, 14)

struct MMCMPBLOCK
{
	uint32le unpk_size;
	uint32le pk_size;
	uint32le xor_chk;
	uint16le sub_blk;
	uint16le flags;
	uint16le tt_entries;
	uint16le num_bits;
};

MPT_BINARY_STRUCT(MMCMPBLOCK, 20)

struct MMCMPSUBBLOCK
{
	uint32le unpk_pos;
	uint32le unpk_size;
};

MPT_BINARY_STRUCT(MMCMPSUBBLOCK, 8)


#define MMCMP_COMP		0x0001
#define MMCMP_DELTA		0x0002
#define MMCMP_16BIT		0x0004
#define MMCMP_STEREO	0x0100
#define MMCMP_ABS16		0x0200
#define MMCMP_ENDIAN	0x0400

static constexpr uint8 MMCMP8BitCommands[8] =
{
	0x01, 0x03, 0x07, 0x0F, 0x1E, 0x3C, 0x78, 0xF8
};

static constexpr uint8 MMCMP8BitFetch[8] =
{
	3, 3, 3, 3, 2, 1, 0, 0
};

static constexpr uint16 MMCMP16BitCommands[16] =
{
	0x01,  0x03,  0x07,  0x0F,  0x1E,   0x3C,   0x78,   0xF0,
	0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFFF0
};

static constexpr uint8 MMCMP16BitFetch[16] =
{
	4, 4, 4, 4, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


static bool MMCMP_IsDstBlockValid(const std::vector<char> &unpackedData, uint32 pos, uint32 len)
{
	if(pos >= unpackedData.size()) return false;
	if(len > unpackedData.size()) return false;
	if(len > unpackedData.size() - pos) return false;
	return true;
}


static bool MMCMP_IsDstBlockValid(const std::vector<char> &unpackedData, const MMCMPSUBBLOCK &subblk)
{
	return MMCMP_IsDstBlockValid(unpackedData, subblk.unpk_pos, subblk.unpk_size);
}


static bool ValidateHeader(const MMCMPFILEHEADER &mfh)
{
	if(std::memcmp(mfh.id, "ziRCONia", 8) != 0)
	{
		return false;
	}
	if(mfh.hdrsize != sizeof(MMCMPHEADER))
	{
		return false;
	}
	return true;
}


static bool ValidateHeader(const MMCMPHEADER &mmh)
{
	if(mmh.nblocks == 0)
	{
		return false;
	}
	if(mmh.filesize == 0)
	{
		return false;
	}
	if(mmh.filesize > 0x80000000)
	{
		return false;
	}
	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMMCMP(MemoryFileReader file, const uint64 *pfilesize)
{
	MMCMPFILEHEADER mfh;
	if(!file.ReadStruct(mfh))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(mfh))
	{
		return ProbeFailure;
	}
	MMCMPHEADER mmh;
	if(!file.ReadStruct(mmh))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(mmh))
	{
		return ProbeFailure;
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool UnpackMMCMP(std::vector<ContainerItem> &containerItems, FileReader &file, ContainerLoadingFlags loadFlags)
{
	file.Rewind();
	containerItems.clear();

	MMCMPFILEHEADER mfh;
	if(!file.ReadStruct(mfh))
	{
		return false;
	}
	if(!ValidateHeader(mfh))
	{
		return false;
	}
	MMCMPHEADER mmh;
	if(!file.ReadStruct(mmh))
	{
		return false;
	}
	if(!ValidateHeader(mmh))
	{
		return false;
	}
	if(loadFlags == ContainerOnlyVerifyHeader)
	{
		return true;
	}
	if(mmh.blktable > file.GetLength()) return false;
	if(mmh.blktable + 4 * mmh.nblocks > file.GetLength()) return false;

	containerItems.emplace_back();
	containerItems.back().data_cache = std::make_unique<std::vector<char> >();
	std::vector<char> & unpackedData = *(containerItems.back().data_cache);

	unpackedData.resize(mmh.filesize);
	// 8-bit deltas
	uint8 ptable[256] = { 0 };

	for (uint32 nBlock=0; nBlock<mmh.nblocks; nBlock++)
	{
		if(!file.Seek(mmh.blktable + 4*nBlock)) return false;
		if(!file.CanRead(4)) return false;
		uint32 blkPos = file.ReadUint32LE();
		if(!file.Seek(blkPos)) return false;
		MMCMPBLOCK blk;
		if(!file.ReadStruct(blk)) return false;
		std::vector<MMCMPSUBBLOCK> subblks(blk.sub_blk);
		for(uint32 i=0; i<blk.sub_blk; ++i)
		{
			if(!file.ReadStruct(subblks[i])) return false;
		}
		const MMCMPSUBBLOCK *psubblk = blk.sub_blk > 0 ? subblks.data() : nullptr;

		if(blkPos + sizeof(MMCMPBLOCK) + blk.sub_blk * sizeof(MMCMPSUBBLOCK) >= file.GetLength()) return false;
		uint32 memPos = blkPos + sizeof(MMCMPBLOCK) + blk.sub_blk * sizeof(MMCMPSUBBLOCK);

#ifdef MMCMP_LOG
		MPT_LOG(LogDebug, "MMCMP", mpt::format(U_("block %1: flags=%2 sub_blocks=%3"))(nBlock, mpt::ufmt::HEX0<4>(static_cast<uint16>(blk.flags)), static_cast<uint16>(blk.sub_blk)));
		MPT_LOG(LogDebug, "MMCMP", mpt::format(U_(" pksize=%1 unpksize=%2"))(static_cast<uint32>(blk.pk_size), static_cast<uint32>(blk.unpk_size)));
		MPT_LOG(LogDebug, "MMCMP", mpt::format(U_(" tt_entries=%1 num_bits=%2"))(static_cast<uint16>(blk.tt_entries), static_cast<uint16>(blk.num_bits)));
#endif
		// Data is not packed
		if (!(blk.flags & MMCMP_COMP))
		{
			for (uint32 i=0; i<blk.sub_blk; i++)
			{
				if(!psubblk) return false;
				if(!MMCMP_IsDstBlockValid(unpackedData, *psubblk)) return false;
#ifdef MMCMP_LOG
				MPT_LOG(LogDebug, "MMCMP", mpt::format(U_("  Unpacked sub-block %1: offset %2, size=%3"))(i, static_cast<uint32>(psubblk->unpk_pos), static_cast<uint32>(psubblk->unpk_size)));
#endif
				if(!file.Seek(memPos)) return false;
				if(file.ReadRaw(&(unpackedData[psubblk->unpk_pos]), psubblk->unpk_size) != psubblk->unpk_size) return false;
				psubblk++;
			}
		} else
		// Data is 16-bit packed
		if (blk.flags & MMCMP_16BIT)
		{
			uint32 subblk = 0;
			if(!psubblk) return false;
			if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
			char *pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
			uint32 dwSize = psubblk[subblk].unpk_size;
			uint32 dwPos = 0;
			uint32 numbits = blk.num_bits;
			uint32 oldval = 0;

#ifdef MMCMP_LOG
			MPT_LOG(LogDebug, "MMCMP", mpt::format(U_("  16-bit block: pos=%1 size=%2 %3 %4"))(psubblk->unpk_pos, psubblk->unpk_size, (blk.flags & MMCMP_DELTA) ? U_("DELTA ") : U_(""), (blk.flags & MMCMP_ABS16) ? U_("ABS16 ") : U_("")));
#endif
			if(!file.Seek(memPos + blk.tt_entries)) return false;
			if(!file.CanRead(blk.pk_size - blk.tt_entries)) return false;
			BitReader bitFile{ file.GetChunk(blk.pk_size - blk.tt_entries) };

			try
			{
				while (subblk < blk.sub_blk)
				{
					uint32 newval = 0x10000;
					uint32 d = bitFile.ReadBits(numbits + 1);

					uint32 command = MMCMP16BitCommands[numbits & 0x0F];
					if (d >= command)
					{
						uint32 nFetch = MMCMP16BitFetch[numbits & 0x0F];
						uint32 newbits = bitFile.ReadBits(nFetch) + ((d - command) << nFetch);
						if (newbits != numbits)
						{
							numbits = newbits & 0x0F;
						} else
						{
							if ((d = bitFile.ReadBits(4)) == 0x0F)
							{
								if (bitFile.ReadBits(1)) break;
								newval = 0xFFFF;
							} else
							{
								newval = 0xFFF0 + d;
							}
						}
					} else
					{
						newval = d;
					}
					if (newval < 0x10000)
					{
						newval = (newval & 1) ? (uint32)(-(int32)((newval+1) >> 1)) : (uint32)(newval >> 1);
						if (blk.flags & MMCMP_DELTA)
						{
							newval += oldval;
							oldval = newval;
						} else
						if (!(blk.flags & MMCMP_ABS16))
						{
							newval ^= 0x8000;
						}
						pDest[dwPos + 0] = (uint8)(((uint16)newval) & 0xFF);
						pDest[dwPos + 1] = (uint8)(((uint16)newval) >> 8);
						dwPos += 2;
					}
					if (dwPos >= dwSize)
					{
						subblk++;
						dwPos = 0;
						if(!(subblk < blk.sub_blk)) break;
						if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
						dwSize = psubblk[subblk].unpk_size;
						pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
					}
				}
			} catch(const BitReader::eof &)
			{
			}
		} else
		// Data is 8-bit packed
		{
			uint32 subblk = 0;
			if(!psubblk) return false;
			if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
			char *pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
			uint32 dwSize = psubblk[subblk].unpk_size;
			uint32 dwPos = 0;
			uint32 numbits = blk.num_bits;
			uint32 oldval = 0;
			if(blk.tt_entries > sizeof(ptable)
				|| !file.Seek(memPos)
				|| file.ReadRaw(ptable, blk.tt_entries) < blk.tt_entries)
				return false;

			if(!file.CanRead(blk.pk_size - blk.tt_entries)) return false;
			BitReader bitFile{ file.GetChunk(blk.pk_size - blk.tt_entries) };

			try
			{
				while (subblk < blk.sub_blk)
				{
					uint32 newval = 0x100;
					uint32 d = bitFile.ReadBits(numbits + 1);

					uint32 command = MMCMP8BitCommands[numbits & 0x07];
					if (d >= command)
					{
						uint32 nFetch = MMCMP8BitFetch[numbits & 0x07];
						uint32 newbits = bitFile.ReadBits(nFetch) + ((d - command) << nFetch);
						if (newbits != numbits)
						{
							numbits = newbits & 0x07;
						} else
						{
							if ((d = bitFile.ReadBits(3)) == 7)
							{
								if (bitFile.ReadBits(1)) break;
								newval = 0xFF;
							} else
							{
								newval = 0xF8 + d;
							}
						}
					} else
					{
						newval = d;
					}
					if (newval < sizeof(ptable))
					{
						int n = ptable[newval];
						if (blk.flags & MMCMP_DELTA)
						{
							n += oldval;
							oldval = n;
						}
						pDest[dwPos++] = (uint8)n;
					}
					if (dwPos >= dwSize)
					{
						subblk++;
						dwPos = 0;
						if(!(subblk < blk.sub_blk)) break;
						if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
						dwSize = psubblk[subblk].unpk_size;
						pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
					}
				}
			} catch(const BitReader::eof &)
			{
			}
		}
	}

	containerItems.back().file = FileReader(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(unpackedData)));

	return true;
}


OPENMPT_NAMESPACE_END
