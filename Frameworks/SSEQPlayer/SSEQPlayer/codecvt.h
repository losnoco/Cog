// This comes from llvm's libcxx project. I've copied the code from there (with very minor modifications) for use with GCC and Clang when libcxx isn't being used.

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_LIBCPP_VERSION)
#pragma once

#include <locale>

namespace std
{

enum codecvt_mode
{
	consume_header = 4,
	generate_header = 2,
	little_endian = 1
};

//                                     Valid UTF ranges
//     UTF-32               UTF-16                          UTF-8               # of code points
//                     first      second       first   second    third   fourth
// 000000 - 00007F  0000 - 007F               00 - 7F                                 127
// 000080 - 0007FF  0080 - 07FF               C2 - DF, 80 - BF                       1920
// 000800 - 000FFF  0800 - 0FFF               E0 - E0, A0 - BF, 80 - BF              2048
// 001000 - 00CFFF  1000 - CFFF               E1 - EC, 80 - BF, 80 - BF             49152
// 00D000 - 00D7FF  D000 - D7FF               ED - ED, 80 - 9F, 80 - BF              2048
// 00D800 - 00DFFF                invalid
// 00E000 - 00FFFF  E000 - FFFF               EE - EF, 80 - BF, 80 - BF              8192
// 010000 - 03FFFF  D800 - D8BF, DC00 - DFFF  F0 - F0, 90 - BF, 80 - BF, 80 - BF   196608
// 040000 - 0FFFFF  D8C0 - DBBF, DC00 - DFFF  F1 - F3, 80 - BF, 80 - BF, 80 - BF   786432
// 100000 - 10FFFF  DBC0 - DBFF, DC00 - DFFF  F4 - F4, 80 - 8F, 80 - BF, 80 - BF    65536

namespace UnicodeConverters
{
	inline codecvt_base::result utf16_to_utf8(const uint16_t *frm, const uint16_t *frm_end, const uint16_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 3)
				return codecvt_base::partial;
			*to_nxt++ = 0xEF;
			*to_nxt++ = 0xBB;
			*to_nxt++ = 0xBF;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint16_t wc1 = *frm_nxt;
			if (wc1 > Maxcode)
				return codecvt_base::error;
			if (wc1 < 0x0080)
			{
				if (to_end - to_nxt < 1)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(wc1);
			}
			else if (wc1 < 0x0800)
			{
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xC0 | (wc1 >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x03F));
			}
			else if (wc1 < 0xD800)
			{
				if (to_end - to_nxt < 3)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xE0 | (wc1 >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x003F));
			}
			else if (wc1 < 0xDC00)
			{
				if (frm_end - frm_nxt < 2)
					return codecvt_base::partial;
				uint16_t wc2 = frm_nxt[1];
				if ((wc2 & 0xFC00) != 0xDC00)
					return codecvt_base::error;
				if (to_end - to_nxt < 4)
					return codecvt_base::partial;
				if (((((static_cast<unsigned long>(wc1) & 0x03C0) >> 6) + 1) << 16) + ((static_cast<unsigned long>(wc1) & 0x003F) << 10) + (wc2 & 0x03FF) > Maxcode)
					return codecvt_base::error;
				++frm_nxt;
				uint8_t z = ((wc1 & 0x03C0) >> 6) + 1;
				*to_nxt++ = static_cast<uint8_t>(0xF0 | (z >> 2));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((z & 0x03) << 4) | ((wc1 & 0x003C) >> 2));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0003) << 4) | ((wc2 & 0x03C0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc2 & 0x003F));
			}
			else if (wc1 < 0xE000)
				return codecvt_base::error;
			else
			{
				if (to_end - to_nxt < 3)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xE0 | (wc1 >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x003F));
			}
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf16_to_utf8(const uint32_t *frm, const uint32_t *frm_end, const uint32_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 3)
				return codecvt_base::partial;
			*to_nxt++ = 0xEF;
			*to_nxt++ = 0xBB;
			*to_nxt++ = 0xBF;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint16_t wc1 = static_cast<uint16_t>(*frm_nxt);
			if (wc1 > Maxcode)
				return codecvt_base::error;
			if (wc1 < 0x0080)
			{
				if (to_end - to_nxt < 1)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(wc1);
			}
			else if (wc1 < 0x0800)
			{
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xC0 | (wc1 >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x03F));
			}
			else if (wc1 < 0xD800)
			{
				if (to_end - to_nxt < 3)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xE0 | (wc1 >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x003F));
			}
			else if (wc1 < 0xDC00)
			{
				if (frm_end - frm_nxt < 2)
					return codecvt_base::partial;
				uint16_t wc2 = static_cast<uint16_t>(frm_nxt[1]);
				if ((wc2 & 0xFC00) != 0xDC00)
					return codecvt_base::error;
				if (to_end - to_nxt < 4)
					return codecvt_base::partial;
				if (((((static_cast<unsigned long>(wc1) & 0x03C0) >> 6) + 1) << 16) + ((static_cast<unsigned long>(wc1) & 0x003F) << 10) + (wc2 & 0x03FF) > Maxcode)
					return codecvt_base::error;
				++frm_nxt;
				uint8_t z = ((wc1 & 0x03C0) >> 6) + 1;
				*to_nxt++ = static_cast<uint8_t>(0xF0 | (z >> 2));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((z & 0x03) << 4) | ((wc1 & 0x003C) >> 2));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0003) << 4) | ((wc2 & 0x03C0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc2 & 0x003F));
			}
			else if (wc1 < 0xE000)
				return codecvt_base::error;
			else
			{
				if (to_end - to_nxt < 3)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xE0 | (wc1 >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x003F));
			}
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf8_to_utf16(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint16_t *to, uint16_t *to_end, uint16_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (; frm_nxt < frm_end && to_nxt < to_end; ++to_nxt)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 > Maxcode)
				return codecvt_base::error;
			if (c1 < 0x80)
			{
				*to_nxt = static_cast<uint16_t>(c1);
				++frm_nxt;
			}
			else if (c1 < 0xC2)
				return codecvt_base::error;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				if ((c2 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return codecvt_base::error;
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 3;
			}
			else if (c1 < 0xF5)
			{
				if (frm_end - frm_nxt < 4)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				uint8_t c4 = frm_nxt[3];
				switch (c1)
				{
					case 0xF0:
						if (c2 < 0x90 || c2 > 0xBF)
							return codecvt_base::error;
						break;
					case 0xF4:
						if ((c2 & 0xF0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
					return codecvt_base::error;
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				if ((((static_cast<unsigned long>(c1) & 7) << 18) + ((static_cast<unsigned long>(c2) & 0x3F) << 12) + ((static_cast<unsigned long>(c3) & 0x3F) << 6) + (c4 & 0x3F)) > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint16_t>(0xD800 | (((((c1 & 0x07) << 2) | ((c2 & 0x30) >> 4)) - 1) << 6) | ((c2 & 0x0F) << 2) | ((c3 & 0x30) >> 4));
				*++to_nxt = static_cast<uint16_t>(0xDC00 | ((c3 & 0x0F) << 6) | (c4 & 0x3F));
				frm_nxt += 4;
			}
			else
				return codecvt_base::error;
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline codecvt_base::result utf8_to_utf16(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint32_t *to, uint32_t *to_end, uint32_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (; frm_nxt < frm_end && to_nxt < to_end; ++to_nxt)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 > Maxcode)
				return codecvt_base::error;
			if (c1 < 0x80)
			{
				*to_nxt = static_cast<uint32_t>(c1);
				++frm_nxt;
			}
			else if (c1 < 0xC2)
				return codecvt_base::error;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				if ((c2 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint32_t>(t);
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return codecvt_base::error;
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint32_t>(t);
				frm_nxt += 3;
			}
			else if (c1 < 0xF5)
			{
				if (frm_end - frm_nxt < 4)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				uint8_t c4 = frm_nxt[3];
				switch (c1)
				{
					case 0xF0:
						if (c2 < 0x90 || c2 > 0xBF)
							return codecvt_base::error;
						break;
					case 0xF4:
						if ((c2 & 0xF0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
					return codecvt_base::error;
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				if ((((static_cast<unsigned long>(c1) & 7) << 18) + ((static_cast<unsigned long>(c2) & 0x3F) << 12) + ((static_cast<unsigned long>(c3) & 0x3F) << 6) + (c4 & 0x3F)) > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint32_t>(0xD800 | (((((c1 & 0x07) << 2) | ((c2 & 0x30) >> 4)) - 1) << 6) | ((c2 & 0x0F) << 2) | ((c3 & 0x30) >> 4));
				*++to_nxt = static_cast<uint32_t>( 0xDC00 | ((c3 & 0x0F) << 6) | (c4 & 0x3F));
				frm_nxt += 4;
			}
			else
				return codecvt_base::error;
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf8_to_utf16_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (size_t nchar16_t = 0; frm_nxt < frm_end && nchar16_t < mx; ++nchar16_t)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 > Maxcode)
				break;
			if (c1 < 0x80)
				++frm_nxt;
			else if (c1 < 0xC2)
				break;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2 || (frm_nxt[1] & 0xC0) != 0x80)
					break;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6) | (frm_nxt[1] & 0x3F));
				if (t > Maxcode)
					break;
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					break;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return static_cast<int>(frm_nxt - frm);
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
				}
				if ((c3 & 0xC0) != 0x80)
					break;
				if ((((c1 & 0x0Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu)) > Maxcode)
					break;
				frm_nxt += 3;
			}
			else if (c1 < 0xF5)
			{
				if (frm_end - frm_nxt < 4 || mx - nchar16_t < 2)
					break;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				uint8_t c4 = frm_nxt[3];
				switch (c1)
				{
					case 0xF0:
						if (c2 < 0x90 || c2 > 0xBF)
							return static_cast<int>(frm_nxt - frm);
						break;
					case 0xF4:
						if ((c2 & 0xF0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
				}
				if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
					break;
				if ((((static_cast<unsigned long>(c1) & 7) << 18) + ((static_cast<unsigned long>(c2) & 0x3F) << 12) + ((static_cast<unsigned long>(c3) & 0x3F) << 6) + (c4 & 0x3F)) > Maxcode)
					break;
				++nchar16_t;
				frm_nxt += 4;
			}
			else
				break;
		}
		return static_cast<int>(frm_nxt - frm);
	}

	inline codecvt_base::result ucs4_to_utf8(const uint32_t *frm, const uint32_t *frm_end, const uint32_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 3)
				return codecvt_base::partial;
			*to_nxt++ = 0xEF;
			*to_nxt++ = 0xBB;
			*to_nxt++ = 0xBF;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint32_t wc = *frm_nxt;
			if ((wc & 0xFFFFF800) == 0x00D800 || wc > Maxcode)
				return codecvt_base::error;
			if (wc < 0x000080)
			{
				if (to_end - to_nxt < 1)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(wc);
			}
			else if (wc < 0x000800)
			{
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xC0 | (wc >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x03F));
			}
			else if (wc < 0x010000)
			{
				if (to_end - to_nxt < 3)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xE0 | (wc >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x0FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x003F));
			}
			else // if (wc < 0x110000)
			{
				if (to_end - to_nxt < 4)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xF0 | (wc >> 18));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x03F000) >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x000FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x00003F));
			}
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf8_to_ucs4(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint32_t *to, uint32_t *to_end, uint32_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (; frm_nxt < frm_end && to_nxt < to_end; ++to_nxt)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 < 0x80)
			{
				if (c1 > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint32_t>(c1);
				++frm_nxt;
			}
			else if (c1 < 0xC2)
				return codecvt_base::error;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				if ((c2 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint32_t t = static_cast<uint32_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return codecvt_base::error;
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint32_t t = static_cast<uint32_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) |  (c3 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 3;
			}
			else if (c1 < 0xF5)
			{
				if (frm_end - frm_nxt < 4)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				uint8_t c4 = frm_nxt[3];
				switch (c1)
				{
					case 0xF0:
						if (c2 < 0x90 || c2 > 0xBF)
							return codecvt_base::error;
						break;
					case 0xF4:
						if ((c2 & 0xF0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint32_t t = static_cast<uint32_t>(((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) |  (c4 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 4;
			}
			else
				return codecvt_base::error;
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf8_to_ucs4_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (size_t nchar32_t = 0; frm_nxt < frm_end && nchar32_t < mx; ++nchar32_t)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 < 0x80)
			{
				if (c1 > Maxcode)
					break;
				++frm_nxt;
			}
			else if (c1 < 0xC2)
				break;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2 || (frm_nxt[1] & 0xC0) != 0x80)
					break;
				if ((((c1 & 0x1Fu) << 6) | (frm_nxt[1] & 0x3Fu)) > Maxcode)
					break;
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					break;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return static_cast<int>(frm_nxt - frm);
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
				}
				if ((c3 & 0xC0) != 0x80)
					break;
				if ((((c1 & 0x0Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu)) > Maxcode)
					break;
				frm_nxt += 3;
			}
			else if (c1 < 0xF5)
			{
				if (frm_end - frm_nxt < 4)
					break;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				uint8_t c4 = frm_nxt[3];
				switch (c1)
				{
					case 0xF0:
						if (c2 < 0x90 || c2 > 0xBF)
							return static_cast<int>(frm_nxt - frm);
						break;
					case 0xF4:
						if ((c2 & 0xF0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
				}
				if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
					break;
				if ((((c1 & 0x07u) << 18) | ((c2 & 0x3Fu) << 12) | ((c3 & 0x3Fu) << 6)  |  (c4 & 0x3Fu)) > Maxcode)
					break;
				frm_nxt += 4;
			}
			else
				break;
		}
		return static_cast<int>(frm_nxt - frm);
	}

	inline codecvt_base::result ucs2_to_utf8(const uint16_t *frm, const uint16_t *frm_end, const uint16_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 3)
				return codecvt_base::partial;
			*to_nxt++ = 0xEF;
			*to_nxt++ = 0xBB;
			*to_nxt++ = 0xBF;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint16_t wc = *frm_nxt;
			if ((wc & 0xF800) == 0xD800 || wc > Maxcode)
				return codecvt_base::error;
			if (wc < 0x0080)
			{
				if (to_end - to_nxt < 1)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(wc);
			}
			else if (wc < 0x0800)
			{
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xC0 | (wc >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x03F));
			}
			else // if (wc <= 0xFFFF)
			{
				if (to_end - to_nxt < 3)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(0xE0 | (wc >> 12));
				*to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x0FC0) >> 6));
				*to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x003F));
			}
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf8_to_ucs2(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint16_t *to, uint16_t *to_end, uint16_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (; frm_nxt < frm_end && to_nxt < to_end; ++to_nxt)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 < 0x80)
			{
				if (c1 > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint16_t>(c1);
				++frm_nxt;
			}
			else if (c1 < 0xC2)
				return codecvt_base::error;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				if ((c2 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					return codecvt_base::partial;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return codecvt_base::error;
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return codecvt_base::error;
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return codecvt_base::error;
				}
				if ((c3 & 0xC0) != 0x80)
					return codecvt_base::error;
				uint16_t t = static_cast<uint16_t>(((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) |  (c3 & 0x3F));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 3;
			}
			else
				return codecvt_base::error;
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf8_to_ucs2_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB && frm_nxt[2] == 0xBF)
			frm_nxt += 3;
		for (size_t nchar32_t = 0; frm_nxt < frm_end && nchar32_t < mx; ++nchar32_t)
		{
			uint8_t c1 = *frm_nxt;
			if (c1 < 0x80)
			{
				if (c1 > Maxcode)
					break;
				++frm_nxt;
			}
			else if (c1 < 0xC2)
				break;
			else if (c1 < 0xE0)
			{
				if (frm_end - frm_nxt < 2 || (frm_nxt[1] & 0xC0) != 0x80)
					break;
				if ((((c1 & 0x1Fu) << 6) | (frm_nxt[1] & 0x3Fu)) > Maxcode)
					break;
				frm_nxt += 2;
			}
			else if (c1 < 0xF0)
			{
				if (frm_end - frm_nxt < 3)
					break;
				uint8_t c2 = frm_nxt[1];
				uint8_t c3 = frm_nxt[2];
				switch (c1)
				{
					case 0xE0:
						if ((c2 & 0xE0) != 0xA0)
							return static_cast<int>(frm_nxt - frm);
						break;
					case 0xED:
						if ((c2 & 0xE0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
						break;
					default:
						if ((c2 & 0xC0) != 0x80)
							return static_cast<int>(frm_nxt - frm);
				}
				if ((c3 & 0xC0) != 0x80)
					break;
				if ((((c1 & 0x0Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu)) > Maxcode)
					break;
				frm_nxt += 3;
			}
			else
				break;
		}
		return static_cast<int>(frm_nxt - frm);
	}

	inline codecvt_base::result ucs4_to_utf16be(const uint32_t *frm, const uint32_t *frm_end, const uint32_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 2)
				return codecvt_base::partial;
			*to_nxt++ = 0xFE;
			*to_nxt++ = 0xFF;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint32_t wc = *frm_nxt;
			if ((wc & 0xFFFFF800) == 0x00D800 || wc > Maxcode)
				return codecvt_base::error;
			if (wc < 0x010000)
			{
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(wc >> 8);
				*to_nxt++ = static_cast<uint8_t>(wc);
			}
			else
			{
				if (to_end - to_nxt < 4)
					return codecvt_base::partial;
				uint16_t t = static_cast<uint16_t>(0xD800 | ((((wc & 0x1F0000) >> 16) - 1) << 6) | ((wc & 0x00FC00) >> 10));
				*to_nxt++ = static_cast<uint8_t>(t >> 8);
				*to_nxt++ = static_cast<uint8_t>(t);
				t = static_cast<uint16_t>(0xDC00 | (wc & 0x03FF));
				*to_nxt++ = static_cast<uint8_t>(t >> 8);
				*to_nxt++ = static_cast<uint8_t>(t);
			}
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf16be_to_ucs4(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint32_t *to, uint32_t *to_end, uint32_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFE && frm_nxt[1] == 0xFF)
			frm_nxt += 2;
		for (; frm_nxt < frm_end - 1 && to_nxt < to_end; ++to_nxt)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[0] << 8) | frm_nxt[1]);
			if ((c1 & 0xFC00) == 0xDC00)
				return codecvt_base::error;
			if ((c1 & 0xFC00) != 0xD800)
			{
				if (c1 > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint32_t>(c1);
				frm_nxt += 2;
			}
			else
			{
				if (frm_end - frm_nxt < 4)
					return codecvt_base::partial;
				uint16_t c2 = static_cast<uint16_t>((frm_nxt[2] << 8) | frm_nxt[3]);
				if ((c2 & 0xFC00) != 0xDC00)
					return codecvt_base::error;
				uint32_t t = static_cast<uint32_t>(((((c1 & 0x03C0) >> 6) + 1) << 16) | ((c1 & 0x003F) << 10) | (c2 & 0x03FF));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 4;
			}
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf16be_to_ucs4_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFE && frm_nxt[1] == 0xFF)
			frm_nxt += 2;
		for (size_t nchar32_t = 0; frm_nxt < frm_end - 1 && nchar32_t < mx; ++nchar32_t)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[0] << 8) | frm_nxt[1]);
			if ((c1 & 0xFC00) == 0xDC00)
				break;
			if ((c1 & 0xFC00) != 0xD800)
			{
				if (c1 > Maxcode)
					break;
				frm_nxt += 2;
			}
			else
			{
				if (frm_end - frm_nxt < 4)
					break;
				uint16_t c2 = static_cast<uint16_t>((frm_nxt[2] << 8) | frm_nxt[3]);
				if ((c2 & 0xFC00) != 0xDC00)
					break;
				uint32_t t = static_cast<uint32_t>(((((c1 & 0x03C0) >> 6) + 1) << 16) | ((c1 & 0x003F) << 10) | (c2 & 0x03FF));
				if (t > Maxcode)
					break;
				frm_nxt += 4;
			}
		}
		return static_cast<int>(frm_nxt - frm);
	}

	inline codecvt_base::result ucs4_to_utf16le(const uint32_t *frm, const uint32_t *frm_end, const uint32_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 2)
				return codecvt_base::partial;
			*to_nxt++ = 0xFF;
			*to_nxt++ = 0xFE;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint32_t wc = *frm_nxt;
			if ((wc & 0xFFFFF800) == 0x00D800 || wc > Maxcode)
				return codecvt_base::error;
			if (wc < 0x010000)
			{
				if (to_end - to_nxt < 2)
					return codecvt_base::partial;
				*to_nxt++ = static_cast<uint8_t>(wc);
				*to_nxt++ = static_cast<uint8_t>(wc >> 8);
			}
			else
			{
				if (to_end - to_nxt < 4)
					return codecvt_base::partial;
				uint16_t t = static_cast<uint16_t>(0xD800 | ((((wc & 0x1F0000) >> 16) - 1) << 6) | ((wc & 0x00FC00) >> 10));
				*to_nxt++ = static_cast<uint8_t>(t);
				*to_nxt++ = static_cast<uint8_t>(t >> 8);
				t = static_cast<uint16_t>(0xDC00 | (wc & 0x03FF));
				*to_nxt++ = static_cast<uint8_t>(t);
				*to_nxt++ = static_cast<uint8_t>(t >> 8);
			}
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf16le_to_ucs4(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint32_t *to, uint32_t *to_end, uint32_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFF && frm_nxt[1] == 0xFE)
			frm_nxt += 2;
		for (; frm_nxt < frm_end - 1 && to_nxt < to_end; ++to_nxt)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[1] << 8) | frm_nxt[0]);
			if ((c1 & 0xFC00) == 0xDC00)
				return codecvt_base::error;
			if ((c1 & 0xFC00) != 0xD800)
			{
				if (c1 > Maxcode)
					return codecvt_base::error;
				*to_nxt = static_cast<uint32_t>(c1);
				frm_nxt += 2;
			}
			else
			{
				if (frm_end - frm_nxt < 4)
					return codecvt_base::partial;
				uint16_t c2 = static_cast<uint16_t>((frm_nxt[3] << 8) | frm_nxt[2]);
				if ((c2 & 0xFC00) != 0xDC00)
					return codecvt_base::error;
				uint32_t t = static_cast<uint32_t>(((((c1 & 0x03C0) >> 6) + 1) << 16) | ((c1 & 0x003F) << 10) | (c2 & 0x03FF));
				if (t > Maxcode)
					return codecvt_base::error;
				*to_nxt = t;
				frm_nxt += 4;
			}
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf16le_to_ucs4_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFF && frm_nxt[1] == 0xFE)
			frm_nxt += 2;
		for (size_t nchar32_t = 0; frm_nxt < frm_end - 1 && nchar32_t < mx; ++nchar32_t)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[1] << 8) | frm_nxt[0]);
			if ((c1 & 0xFC00) == 0xDC00)
				break;
			if ((c1 & 0xFC00) != 0xD800)
			{
				if (c1 > Maxcode)
					break;
				frm_nxt += 2;
			}
			else
			{
				if (frm_end - frm_nxt < 4)
					break;
				uint16_t c2 = static_cast<uint16_t>((frm_nxt[3] << 8) | frm_nxt[2]);
				if ((c2 & 0xFC00) != 0xDC00)
					break;
				uint32_t t = static_cast<uint32_t>(((((c1 & 0x03C0) >> 6) + 1) << 16) | ((c1 & 0x003F) << 10) | (c2 & 0x03FF));
				if (t > Maxcode)
					break;
				frm_nxt += 4;
			}
		}
		return static_cast<int>(frm_nxt - frm);
	}

	inline codecvt_base::result ucs2_to_utf16be(const uint16_t *frm, const uint16_t *frm_end, const uint16_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 2)
				return codecvt_base::partial;
			*to_nxt++ = 0xFE;
			*to_nxt++ = 0xFF;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint16_t wc = *frm_nxt;
			if ((wc & 0xF800) == 0xD800 || wc > Maxcode)
				return codecvt_base::error;
			if (to_end - to_nxt < 2)
				return codecvt_base::partial;
			*to_nxt++ = static_cast<uint8_t>(wc >> 8);
			*to_nxt++ = static_cast<uint8_t>(wc);
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf16be_to_ucs2(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint16_t *to, uint16_t *to_end, uint16_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFE && frm_nxt[1] == 0xFF)
			frm_nxt += 2;
		for (; frm_nxt < frm_end - 1 && to_nxt < to_end; ++to_nxt)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[0] << 8) | frm_nxt[1]);
			if ((c1 & 0xF800) == 0xD800 || c1 > Maxcode)
				return codecvt_base::error;
			*to_nxt = c1;
			frm_nxt += 2;
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf16be_to_ucs2_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFE && frm_nxt[1] == 0xFF)
			frm_nxt += 2;
		for (size_t nchar16_t = 0; frm_nxt < frm_end - 1 && nchar16_t < mx; ++nchar16_t)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[0] << 8) | frm_nxt[1]);
			if ((c1 & 0xF800) == 0xD800 || c1 > Maxcode)
				break;
			frm_nxt += 2;
		}
		return static_cast<int>(frm_nxt - frm);
	}

	inline codecvt_base::result ucs2_to_utf16le(const uint16_t *frm, const uint16_t *frm_end, const uint16_t *&frm_nxt, uint8_t *to, uint8_t *to_end, uint8_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if (mode & generate_header)
		{
			if (to_end - to_nxt < 2)
				return codecvt_base::partial;
			*to_nxt++ = 0xFF;
			*to_nxt++ = 0xFE;
		}
		for (; frm_nxt < frm_end; ++frm_nxt)
		{
			uint16_t wc = *frm_nxt;
			if ((wc & 0xF800) == 0xD800 || wc > Maxcode)
				return codecvt_base::error;
			if (to_end - to_nxt < 2)
				return codecvt_base::partial;
			*to_nxt++ = static_cast<uint8_t>(wc);
			*to_nxt++ = static_cast<uint8_t>(wc >> 8);
		}
		return codecvt_base::ok;
	}

	inline codecvt_base::result utf16le_to_ucs2(const uint8_t *frm, const uint8_t *frm_end, const uint8_t *&frm_nxt, uint16_t *to, uint16_t *to_end, uint16_t *&to_nxt, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		frm_nxt = frm;
		to_nxt = to;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFF && frm_nxt[1] == 0xFE)
			frm_nxt += 2;
		for (; frm_nxt < frm_end - 1 && to_nxt < to_end; ++to_nxt)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[1] << 8) | frm_nxt[0]);
			if ((c1 & 0xF800) == 0xD800 || c1 > Maxcode)
				return codecvt_base::error;
			*to_nxt = c1;
			frm_nxt += 2;
		}
		return frm_nxt < frm_end ? codecvt_base::partial : codecvt_base::ok;
	}

	inline int utf16le_to_ucs2_length(const uint8_t *frm, const uint8_t *frm_end, size_t mx, unsigned long Maxcode = 0x10FFFF, codecvt_mode mode = static_cast<codecvt_mode>(0))
	{
		auto frm_nxt = frm;
		if ((mode & consume_header) && frm_end - frm_nxt >= 2 && frm_nxt[0] == 0xFF && frm_nxt[1] == 0xFE)
			frm_nxt += 2;
		for (size_t nchar16_t = 0; frm_nxt < frm_end - 1 && nchar16_t < mx; ++nchar16_t)
		{
			uint16_t c1 = static_cast<uint16_t>((frm_nxt[1] << 8) | frm_nxt[0]);
			if ((c1 & 0xF800) == 0xD800 || c1 > Maxcode)
				break;
			frm_nxt += 2;
		}
		return static_cast<int>(frm_nxt - frm);
	}
}

template<> class codecvt<char16_t, char, mbstate_t> : public locale::facet, public codecvt_base
{
public:
	typedef char16_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit codecvt(size_t __refs = 0) : locale::facet(__refs) { }

	result out(state_type &__st, const intern_type *__frm, const intern_type *__frm_end, const intern_type *&__frm_nxt, extern_type *__to, extern_type *__to_end, extern_type *&__to_nxt) const
	{
		return this->do_out(__st, __frm, __frm_end, __frm_nxt, __to, __to_end, __to_nxt);
	}

	result unshift(state_type &__st, extern_type *__to, extern_type *__to_end, extern_type *&__to_nxt) const
	{
		return this->do_unshift(__st, __to, __to_end, __to_nxt);
	}

	result in(state_type &__st, const extern_type *__frm, const extern_type *__frm_end, const extern_type *&__frm_nxt, intern_type *__to, intern_type *__to_end, intern_type *&__to_nxt) const
	{
		return this->do_in(__st, __frm, __frm_end, __frm_nxt, __to, __to_end, __to_nxt);
	}

	int encoding() const noexcept
	{
		return this->do_encoding();
	}

	bool always_noconv() const noexcept
	{
		return this->do_always_noconv();
	}

	int length(state_type &__st, const extern_type *__frm, const extern_type *__end, size_t __mx) const
	{
		return this->do_length(__st, __frm, __end, __mx);
	}

	int max_length() const noexcept
	{
		return this->do_max_length();
	}

	static locale::id id;

protected:
	explicit codecvt(const char *, size_t __refs = 0) : locale::facet(__refs) { }

	~codecvt() { }

	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint16_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint16_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint16_t *>(to);
		auto _to_end = reinterpret_cast<uint16_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_utf16(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_utf16_length(_frm, _frm_end, mx);
	}
	virtual int do_max_length() const noexcept { return 4; }
};

template<> class codecvt<char32_t, char, mbstate_t> : public locale::facet, public codecvt_base
{
public:
	typedef char32_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit codecvt(size_t __refs = 0) : locale::facet(__refs) { }

	result out(state_type &__st, const intern_type *__frm, const intern_type *__frm_end, const intern_type *&__frm_nxt, extern_type *__to, extern_type *__to_end, extern_type *&__to_nxt) const
	{
		return this->do_out(__st, __frm, __frm_end, __frm_nxt, __to, __to_end, __to_nxt);
	}

	result unshift(state_type &__st, extern_type *__to, extern_type *__to_end, extern_type *&__to_nxt) const
	{
		return this->do_unshift(__st, __to, __to_end, __to_nxt);
	}

	result in(state_type &__st, const extern_type *__frm, const extern_type *__frm_end, const extern_type *&__frm_nxt, intern_type *__to, intern_type *__to_end, intern_type *&__to_nxt) const
	{
		return this->do_in(__st, __frm, __frm_end, __frm_nxt, __to, __to_end, __to_nxt);
	}

	int encoding() const noexcept
	{
		return this->do_encoding();
	}

	bool always_noconv() const noexcept
	{
		return this->do_always_noconv();
	}

	int length(state_type &__st, const extern_type *__frm, const extern_type *__end, size_t __mx) const
	{
		return this->do_length(__st, __frm, __end, __mx);
	}

	int max_length() const noexcept
	{
		return this->do_max_length();
	}

	static locale::id id;

protected:
	explicit codecvt(const char *, size_t __refs = 0) : locale::facet(__refs) { }

	~codecvt() { }

	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs4_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_ucs4_length(_frm, _frm_end, mx);
	}
	virtual int do_max_length() const noexcept { return 4; }
};

template<class _Elem> class __codecvt_utf8;

template<> class __codecvt_utf8<wchar_t> : public codecvt<wchar_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef wchar_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf8(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<wchar_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
#ifdef _WIN32
		auto _frm = reinterpret_cast<const uint16_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint16_t *>(frm_end);
#else
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
#endif
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::
#ifdef _WIN32
			ucs2_to_utf8
#else
			ucs4_to_utf8
#endif
			(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
#ifdef _WIN32
		auto _to = reinterpret_cast<uint16_t *>(to);
		auto _to_end = reinterpret_cast<uint16_t *>(to_end);
#else
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
#endif
		auto _to_nxt = _to;
		auto r = UnicodeConverters::
#ifdef _WIN32
			utf8_to_ucs2
#else
			utf8_to_ucs4
#endif
			(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_ucs4_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 7;
		return 4;
	}
};

template<> class __codecvt_utf8<char16_t> : public codecvt<char16_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char16_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf8(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char16_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint16_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint16_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs2_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint16_t *>(to);
		auto _to_end = reinterpret_cast<uint16_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_ucs2(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_ucs2_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 6;
		return 3;
	}
};

template<> class __codecvt_utf8<char32_t> : public codecvt<char32_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char32_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf8(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char32_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs4_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_ucs4_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 7;
		return 4;
	}
};

template<class _Elem, unsigned long _Maxcode = 0x10ffff, codecvt_mode _Mode = static_cast<codecvt_mode>(0)> class codecvt_utf8 : public __codecvt_utf8<_Elem>
{
public:
	explicit codecvt_utf8(size_t __refs = 0) : __codecvt_utf8<_Elem>(__refs, _Maxcode, _Mode) { }

	~codecvt_utf8() { }
};

template<class _Elem, bool _LittleEndian> class __codecvt_utf16;

template<> class __codecvt_utf16<wchar_t, false> : public codecvt<wchar_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef wchar_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<wchar_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs4_to_utf16be(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16be_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf16be_to_ucs4_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 6;
		return 4;
	}
};

template<> class __codecvt_utf16<wchar_t, true> : public codecvt<wchar_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef wchar_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<wchar_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs4_to_utf16le(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16le_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf16le_to_ucs4_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 6;
		return 4;
	}
};

template<> class __codecvt_utf16<char16_t, false> : public codecvt<char16_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char16_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char16_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint16_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint16_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs2_to_utf16be(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint16_t *>(to);
		auto _to_end = reinterpret_cast<uint16_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16be_to_ucs2(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf16be_to_ucs2_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 4;
		return 2;
	}
};

template<> class __codecvt_utf16<char16_t, true> : public codecvt<char16_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char16_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char16_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint16_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint16_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs2_to_utf16le(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint16_t *>(to);
		auto _to_end = reinterpret_cast<uint16_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16le_to_ucs2(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf16le_to_ucs2_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 4;
		return 2;
	}
};

template<> class __codecvt_utf16<char32_t, false> : public codecvt<char32_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char32_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char32_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs4_to_utf16be(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16be_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf16be_to_ucs4_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 6;
		return 4;
	}
};

template<> class __codecvt_utf16<char32_t, true> : public codecvt<char32_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char32_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char32_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::ucs4_to_utf16le(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16le_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf16le_to_ucs4_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 6;
		return 4;
	}
};

template<class _Elem, unsigned long _Maxcode = 0x10ffff, codecvt_mode _Mode = static_cast<codecvt_mode>(0)> class codecvt_utf16 : public __codecvt_utf16<_Elem, _Mode & little_endian>
{
public:
	explicit codecvt_utf16(size_t __refs = 0) : __codecvt_utf16<_Elem, _Mode & little_endian>(__refs, _Maxcode, _Mode) { }

	~codecvt_utf16() { }
};

template<class _Elem> class __codecvt_utf8_utf16;

template<> class __codecvt_utf8_utf16<wchar_t> : public codecvt<wchar_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef wchar_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf8_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<wchar_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_utf16(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_utf16_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 7;
		return 4;
	}
};

template<> class __codecvt_utf8_utf16<char32_t> : public codecvt<char32_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char32_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf8_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char32_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint32_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint32_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint32_t *>(to);
		auto _to_end = reinterpret_cast<uint32_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_utf16(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_utf16_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 7;
		return 4;
	}
};

template<> class __codecvt_utf8_utf16<char16_t> : public codecvt<char16_t, char, mbstate_t>
{
	unsigned long _Maxcode_;
	codecvt_mode _Mode_;
public:
	typedef char16_t intern_type;
	typedef char extern_type;
	typedef mbstate_t state_type;

	explicit __codecvt_utf8_utf16(size_t __refs, unsigned long _Maxcode, codecvt_mode _Mode) : codecvt<char16_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode), _Mode_(_Mode) { }
protected:
	virtual result do_out(state_type &, const intern_type *frm, const intern_type *frm_end, const intern_type *&frm_nxt, extern_type *to, extern_type *to_end, extern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint16_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint16_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint8_t *>(to);
		auto _to_end = reinterpret_cast<uint8_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf16_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_in(state_type &, const extern_type *frm, const extern_type *frm_end, const extern_type *&frm_nxt, intern_type *to, intern_type *to_end, intern_type *&to_nxt) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		auto _frm_nxt = _frm;
		auto _to = reinterpret_cast<uint16_t *>(to);
		auto _to_end = reinterpret_cast<uint16_t *>(to_end);
		auto _to_nxt = _to;
		auto r = UnicodeConverters::utf8_to_utf16(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt, this->_Maxcode_, this->_Mode_);
		frm_nxt = frm + (_frm_nxt - _frm);
		to_nxt = to + (_to_nxt - _to);
		return r;
	}
	virtual result do_unshift(state_type &, extern_type *to, extern_type *, extern_type *&to_nxt) const
	{
		to_nxt = to;
		return noconv;
	}
	virtual int do_encoding() const noexcept { return 0; }
	virtual bool do_always_noconv() const noexcept { return false; }
	virtual int do_length(state_type &, const extern_type *frm, const extern_type *frm_end, size_t mx) const
	{
		auto _frm = reinterpret_cast<const uint8_t *>(frm);
		auto _frm_end = reinterpret_cast<const uint8_t *>(frm_end);
		return UnicodeConverters::utf8_to_utf16_length(_frm, _frm_end, mx, this->_Maxcode_, this->_Mode_);
	}
	virtual int do_max_length() const noexcept
	{
		if (this->_Mode_ & consume_header)
			return 7;
		return 4;
	}
};

template<class _Elem, unsigned long _Maxcode = 0x10ffff, codecvt_mode _Mode = static_cast<codecvt_mode>(0)> class codecvt_utf8_utf16 : public __codecvt_utf8_utf16<_Elem>
{
public:
	explicit codecvt_utf8_utf16(size_t __refs = 0) : __codecvt_utf8_utf16<_Elem>(__refs, _Maxcode, _Mode) { }

	~codecvt_utf8_utf16() {}
};

}
#endif
