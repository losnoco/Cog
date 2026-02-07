// snes_spc 0.9.0. http://www.slack.net/~ant/

#include <algorithm>
#include <cstring>
#include "SPC_DSP.h"

#include "blargg_endianSNSF.h"

/* Copyright (C) 2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#if INT_MAX < 0x7FFFFFFF
# error "Requires that int type have at least 32 bits"
#endif

static const uint8_t initial_regs[] =
{
	0x45, 0x8B, 0x5A, 0x9A, 0xE4, 0x82, 0x1B, 0x78, 0x00, 0x00, 0xAA, 0x96, 0x89, 0x0E, 0xE0, 0x80,
	0x2A, 0x49, 0x3D, 0xBA, 0x14, 0xA0, 0xAC, 0xC5, 0x00, 0x00, 0x51, 0xBB, 0x9C, 0x4E, 0x7B, 0xFF,
	0xF4, 0xFD, 0x57, 0x32, 0x37, 0xD9, 0x42, 0x22, 0x00, 0x00, 0x5B, 0x3C, 0x9F, 0x1B, 0x87, 0x9A,
	0x6F, 0x27, 0xAF, 0x7B, 0xE5, 0x68, 0x0A, 0xD9, 0x00, 0x00, 0x9A, 0xC5, 0x9C, 0x4E, 0x7B, 0xFF,
	0xEA, 0x21, 0x78, 0x4F, 0xDD, 0xED, 0x24, 0x14, 0x00, 0x00, 0x77, 0xB1, 0xD1, 0x36, 0xC1, 0x67,
	0x52, 0x57, 0x46, 0x3D, 0x59, 0xF4, 0x87, 0xA4, 0x00, 0x00, 0x7E, 0x44, 0x00, 0x4E, 0x7B, 0xFF,
	0x75, 0xF5, 0x06, 0x97, 0x10, 0xC3, 0x24, 0xBB, 0x00, 0x00, 0x7B, 0x7A, 0xE0, 0x60, 0x12, 0x0F,
	0xF7, 0x74, 0x1C, 0xE5, 0x39, 0x3D, 0x73, 0xC1, 0x00, 0x00, 0x7A, 0xB3, 0xFF, 0x4E, 0x7B, 0xFF
};

// if ( io < -32768 ) io = -32768;
// if ( io >  32767 ) io =  32767;
template<typename T> static inline void CLAMP16(T &io)
{
	if (static_cast<int16_t>(io) != io)
		io = (io >> 31) ^ 0x7FFF;
}

void SPC_DSP::set_output(sample_t *out, int size)
{
	assert(!(size & 1)); // must be even
	if (!out)
	{
		out = this->m.extra;
		size = extra_size;
	}
	this->m.out_begin = this->m.out = out;
	this->m.out_end = out + size;
}

// Volume registers and efb are signed! Easy to forget int8_t cast.
// Prefixes are to avoid accidental use of locals with same names.

// Gaussian interpolation

static const short gauss[] =
{
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
	   2,   2,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,
	   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  10,
	  11,  11,  11,  12,  12,  13,  13,  14,  14,  15,  15,  15,  16,  16,  17,  17,
	  18,  19,  19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,  26,  27,  27,
	  28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  36,  36,  37,  38,  39,  40,
	  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,
	  58,  59,  60,  61,  62,  64,  65,  66,  67,  69,  70,  71,  73,  74,  76,  77,
	  78,  80,  81,  83,  84,  86,  87,  89,  90,  92,  94,  95,  97,  99, 100, 102,
	 104, 106, 107, 109, 111, 113, 115, 117, 118, 120, 122, 124, 126, 128, 130, 132,
	 134, 137, 139, 141, 143, 145, 147, 150, 152, 154, 156, 159, 161, 163, 166, 168,
	 171, 173, 175, 178, 180, 183, 186, 188, 191, 193, 196, 199, 201, 204, 207, 210,
	 212, 215, 218, 221, 224, 227, 230, 233, 236, 239, 242, 245, 248, 251, 254, 257,
	 260, 263, 267, 270, 273, 276, 280, 283, 286, 290, 293, 297, 300, 304, 307, 311,
	 314, 318, 321, 325, 328, 332, 336, 339, 343, 347, 351, 354, 358, 362, 366, 370,
	 374, 378, 381, 385, 389, 393, 397, 401, 405, 410, 414, 418, 422, 426, 430, 434,
	 439, 443, 447, 451, 456, 460, 464, 469, 473, 477, 482, 486, 491, 495, 499, 504,
	 508, 513, 517, 522, 527, 531, 536, 540, 545, 550, 554, 559, 563, 568, 573, 577,
	 582, 587, 592, 596, 601, 606, 611, 615, 620, 625, 630, 635, 640, 644, 649, 654,
	 659, 664, 669, 674, 678, 683, 688, 693, 698, 703, 708, 713, 718, 723, 728, 732,
	 737, 742, 747, 752, 757, 762, 767, 772, 777, 782, 787, 792, 797, 802, 806, 811,
	 816, 821, 826, 831, 836, 841, 846, 851, 855, 860, 865, 870, 875, 880, 884, 889,
	 894, 899, 904, 908, 913, 918, 923, 927, 932, 937, 941, 946, 951, 955, 960, 965,
	 969, 974, 978, 983, 988, 992, 997,1001,1005,1010,1014,1019,1023,1027,1032,1036,
	1040,1045,1049,1053,1057,1061,1066,1070,1074,1078,1082,1086,1090,1094,1098,1102,
	1106,1109,1113,1117,1121,1125,1128,1132,1136,1139,1143,1146,1150,1153,1157,1160,
	1164,1167,1170,1174,1177,1180,1183,1186,1190,1193,1196,1199,1202,1205,1207,1210,
	1213,1216,1219,1221,1224,1227,1229,1232,1234,1237,1239,1241,1244,1246,1248,1251,
	1253,1255,1257,1259,1261,1263,1265,1267,1269,1270,1272,1274,1275,1277,1279,1280,
	1282,1283,1284,1286,1287,1288,1290,1291,1292,1293,1294,1295,1296,1297,1297,1298,
	1299,1300,1300,1301,1302,1302,1303,1303,1303,1304,1304,1304,1304,1304,1305,1305,
};

int SPC_DSP::interpolate(const voice_t *v)
{
	// Make pointers into gaussian based on fractional position between samples
	int offset = (v->interp_pos >> 4) & 0xFF;
	auto fwd = gauss + 255 - offset;
	auto rev = gauss + offset; // mirror left half of gaussian

	auto in = &v->buf[(v->interp_pos >> 12) + v->buf_pos];
	int out = (fwd[0] * in[0]) >> 11;
	out += (fwd[256] * in[1]) >> 11;
	out += (rev[256] * in[2]) >> 11;
	out = static_cast<int16_t>(out);
	out += (rev[0] * in[3]) >> 11;

	CLAMP16(out);
	out &= ~1;
	return out;
}

//// Counters

static const int simple_counter_range = 2048 * 5 * 3; // 30720

static const unsigned counter_rates[] =
{
	simple_counter_range + 1, // never fires
	2048, 1536,
	1280, 1024, 768,
	640, 512, 384,
	320, 256, 192,
	160, 128, 96,
	80, 64, 48,
	40, 32, 24,
	20, 16, 12,
	10, 8, 6,
	5, 4, 3,
	2,
	1
};

static const unsigned counter_offsets[] =
{
	1, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	536, 0, 1040,
	0,
	0
};

void SPC_DSP::init_counter()
{
	this->m.counter = 0;
}

void SPC_DSP::run_counters()
{
	if (--this->m.counter < 0)
		this->m.counter = simple_counter_range - 1;
}

unsigned SPC_DSP::read_counter(int rate)
{
	return (static_cast<unsigned>(this->m.counter) + counter_offsets[rate]) % counter_rates[rate];
}

//// Envelope

void SPC_DSP::run_envelope(voice_t *const v)
{
	int env = v->env;
	if (v->env_mode == env_release) // 60%
	{
		if ((env -= 0x8) < 0)
			env = 0;
		v->env = env;
	}
	else
	{
		int rate;
		int env_data = v->regs[v_adsr1];
		if (this->m.t_adsr0 & 0x80) // 99% ADSR
		{
			if (v->env_mode >= env_decay) // 99%
			{
				--env;
				env -= env >> 8;
				rate = env_data & 0x1F;
				if (v->env_mode == env_decay) // 1%
					rate = ((this->m.t_adsr0 >> 3) & 0x0E) + 0x10;
			}
			else // env_attack
			{
				rate = (this->m.t_adsr0 & 0x0F) * 2 + 1;
				env += rate < 31 ? 0x20 : 0x400;
			}
		}
		else // GAIN
		{
			env_data = v->regs[v_gain];
			int mode = env_data >> 5;
			if (mode < 4) // direct
			{
				env = env_data * 0x10;
				rate = 31;
			}
			else
			{
				rate = env_data & 0x1F;
				if (mode == 4) // 4: linear decrease
					env -= 0x20;
				else if (mode < 6) // 5: exponential decrease
				{
					--env;
					env -= env >> 8;
				}
				else // 6,7: linear increase
				{
					env += 0x20;
					if (mode > 6 && static_cast<unsigned>(v->hidden_env) >= 0x600)
						env += 0x8 - 0x20; // 7: two-slope linear increase
				}
			}
		}

		// Sustain level
		if ((env >> 8) == (env_data >> 5) && v->env_mode == env_decay)
			v->env_mode = env_sustain;

		v->hidden_env = env;

		// unsigned cast because linear decrease going negative also triggers this
		if (static_cast<unsigned>(env) > 0x7FF)
		{
			env = env < 0 ? 0 : 0x7FF;
			if (v->env_mode == env_attack)
				v->env_mode = env_decay;
		}

		if (!this->read_counter(rate))
			v->env = env; // nothing else is controlled by the counter
	}
}

//// BRR Decoding

void SPC_DSP::decode_brr(voice_t *v)
{
	// Arrange the four input nybbles in 0xABCD order for easy decoding
	int nybbles = this->m.t_brr_byte * 0x100 + this->m.ram[(v->brr_addr + v->brr_offset + 1) & 0xFFFF];

	int header = this->m.t_brr_header;

	// Write to next four samples in circular buffer
	int *pos = &v->buf[v->buf_pos];
	int *end;
	if ((v->buf_pos += 4) >= brr_buf_size)
		v->buf_pos = 0;

	// Decode four samples
	for (end = pos + 4; pos < end; ++pos, nybbles <<= 4)
	{
		// Extract nybble and sign-extend
		int s = static_cast<int16_t>(nybbles) >> 12;

		// Shift sample based on header
		int shift = header >> 4;
		s = (s << shift) >> 1;
		if (shift >= 0xD) // handle invalid range
			s = (s >> 25) << 11; // same as: s = (s < 0 ? -0x800 : 0)

		// Apply IIR filter (8 is the most commonly used)
		int filter = header & 0x0C;
		int p1 = pos[brr_buf_size - 1];
		int p2 = pos[brr_buf_size - 2] >> 1;
		if (filter >= 8)
		{
			s += p1;
			s -= p2;
			if (filter == 8) // s += p1 * 0.953125 - p2 * 0.46875
			{
				s += p2 >> 4;
				s += (p1 * -3) >> 6;
			}
			else // s += p1 * 0.8984375 - p2 * 0.40625
			{
				s += (p1 * -13) >> 7;
				s += (p2 * 3) >> 4;
			}
		}
		else if (filter) // s += p1 * 0.46875
		{
			s += p1 >> 1;
			s += -p1 >> 5;
		}

		// Adjust and write sample
		CLAMP16(s);
		s = static_cast<int16_t>(s * 2);
		pos[brr_buf_size] = pos[0] = s; // second copy simplifies wrap-around
	}
}

//// Misc

void SPC_DSP::misc_27()
{
	this->m.t_pmon = this->m.regs[r_pmon] & 0xFE; // voice 0 doesn't support PMON
}
void SPC_DSP::misc_28()
{
	this->m.t_non = this->m.regs[r_non];
	this->m.t_eon = this->m.regs[r_eon];
	this->m.t_dir = this->m.regs[r_dir];
}
void SPC_DSP::misc_29()
{
	this->m.every_other_sample = !this->m.every_other_sample;
	if (this->m.every_other_sample)
		this->m.new_kon &= ~this->m.kon; // clears KON 63 clocks after it was last read
}
void SPC_DSP::misc_30()
{
	if (this->m.every_other_sample)
	{
		this->m.kon = this->m.new_kon;
		this->m.t_koff = this->m.regs[r_koff] | this->m.mute_mask;
	}

	this->run_counters();

	// Noise
	if (!this->read_counter(this->m.regs[r_flg] & 0x1F))
	{
		int feedback = (this->m.noise << 13) ^ (this->m.noise << 14);
		this->m.noise = (feedback & 0x4000) ^ (this->m.noise >> 1);
	}
}

//// Voices

void SPC_DSP::voice_V1(voice_t *const v)
{
	this->m.t_dir_addr = this->m.t_dir * 0x100 + this->m.t_srcn * 4;
	this->m.t_srcn = v->regs[v_srcn];
}
void SPC_DSP::voice_V2(voice_t *const v)
{
	// Read sample pointer (ignored if not needed)
	auto entry = &this->m.ram[m.t_dir_addr];
	if (!v->kon_delay)
		entry += 2;
	this->m.t_brr_next_addr = get_le16(entry);

	this->m.t_adsr0 = v->regs[v_adsr0];

	// Read pitch, spread over two clocks
	this->m.t_pitch = v->regs[v_pitchl];
}
void SPC_DSP::voice_V3a(voice_t *const v)
{
	this->m.t_pitch += (v->regs[v_pitchh] & 0x3F) << 8;
}
void SPC_DSP::voice_V3b(voice_t *const v)
{
	// Read BRR header and byte
	this->m.t_brr_byte = this->m.ram[(v->brr_addr + v->brr_offset) & 0xFFFF];
	this->m.t_brr_header = this->m.ram[v->brr_addr]; // brr_addr doesn't need masking
}
void SPC_DSP::voice_V3c(voice_t *const v)
{
	// Pitch modulation using previous voice's output
	if (this->m.t_pmon & v->vbit)
		this->m.t_pitch += ((this->m.t_output >> 5) * this->m.t_pitch) >> 10;

	if (v->kon_delay)
	{
		// Get ready to start BRR decoding on next sample
		if (v->kon_delay == 5)
		{
			v->brr_addr = this->m.t_brr_next_addr;
			v->brr_offset = 1;
			v->buf_pos = 0;
			this->m.t_brr_header = 0; // header is ignored on this sample
		}

		// Envelope is never run during KON
		v->env = 0;
		v->hidden_env = 0;

		// Disable BRR decoding until last three samples
		v->interp_pos = 0;
		if (--v->kon_delay & 3)
			v->interp_pos = 0x4000;

		// Pitch is never added during KON
		this->m.t_pitch = 0;
	}

	// Gaussian interpolation
	int output = this->interpolate(v);

	// Noise
	if (this->m.t_non & v->vbit)
		output = static_cast<int16_t>(this->m.noise * 2);

	// Apply envelope
	this->m.t_output = (output * v->env) >> 11 & ~1;
	v->t_envx_out = static_cast<uint8_t>(v->env >> 4);

	// Immediate silence due to end of sample or soft reset
	if (this->m.regs[r_flg] & 0x80 || (this->m.t_brr_header & 3) == 1)
	{
		v->env_mode = env_release;
		v->env = 0;
	}

	if (this->m.every_other_sample)
	{
		// KOFF
		if (this->m.t_koff & v->vbit)
			v->env_mode = env_release;

		// KON
		if (this->m.kon & v->vbit)
		{
			v->kon_delay = 5;
			v->env_mode  = env_attack;
		}
	}

	// Run envelope for next sample
	if (!v->kon_delay)
		this->run_envelope(v);
}

void SPC_DSP::voice_output(const voice_t *v, int ch)
{
	// Apply left/right volume
	int amp = (this->m.t_output * static_cast<int8_t>(v->regs[v_voll + ch])) >> 7;
	amp *= (this->stereo_switch & (1 << (v->voice_number + ch * voice_count))) ? 1 : 0;

	// Add to output total
	this->m.t_main_out[ch] += amp;
	CLAMP16(this->m.t_main_out[ch]);

	// Optionally add to echo total
	if (this->m.t_eon & v->vbit)
	{
		this->m.t_echo_out[ch] += amp;
		CLAMP16(this->m.t_echo_out[ch]);
	}
}
void SPC_DSP::voice_V4(voice_t *const v)
{
	// Decode BRR
	this->m.t_looped = 0;
	if (v->interp_pos >= 0x4000)
	{
		this->decode_brr(v);

		if ((v->brr_offset += 2) >= brr_block_size)
		{
			// Start decoding next BRR block
			assert(v->brr_offset == brr_block_size);
			v->brr_addr = (v->brr_addr + brr_block_size) & 0xFFFF;
			if (this->m.t_brr_header & 1)
			{
				v->brr_addr = this->m.t_brr_next_addr;
				this->m.t_looped = v->vbit;
			}
			v->brr_offset = 1;
		}
	}

	// Apply pitch
	v->interp_pos = (v->interp_pos & 0x3FFF) + m.t_pitch;

	// Keep from getting too far ahead (when using pitch modulation)
	if (v->interp_pos > 0x7FFF)
		v->interp_pos = 0x7FFF;

	// Output left
	this->voice_output(v, 0);
}
void SPC_DSP::voice_V5(voice_t *const v)
{
	// Output right
	this->voice_output(v, 1);

	// ENDX, OUTX, and ENVX won't update if you wrote to them 1-2 clocks earlier
	int endx_buf = this->m.regs[r_endx] | this->m.t_looped;

	// Clear bit in ENDX if KON just began
	if (v->kon_delay == 5)
		endx_buf &= ~v->vbit;
	this->m.endx_buf = static_cast<uint8_t>(endx_buf);
}
void SPC_DSP::voice_V6(voice_t *const)
{
	this->m.outx_buf = static_cast<uint8_t>(this->m.t_output >> 8);
}
void SPC_DSP::voice_V7(voice_t *const v)
{
	// Update ENDX
	this->m.regs[r_endx] = this->m.endx_buf;

	this->m.envx_buf = v->t_envx_out;
}
void SPC_DSP::voice_V8(voice_t *const v)
{
	// Update OUTX
	v->regs[v_outx] = this->m.outx_buf;
}
void SPC_DSP::voice_V9(voice_t *const v)
{
	// Update ENVX
	v->regs[v_envx] = this->m.envx_buf;
}

// Most voices do all these in one clock, so make a handy composite
void SPC_DSP::voice_V3(voice_t *const v)
{
	this->voice_V3a(v);
	this->voice_V3b(v);
	this->voice_V3c(v);
}

// Common combinations of voice steps on different voices. This greatly reduces
// code size and allows everything to be inlined in these functions.
void SPC_DSP::voice_V7_V4_V1(voice_t *const v) { this->voice_V7(v); this->voice_V1(v + 3); this->voice_V4(v + 1); }
void SPC_DSP::voice_V8_V5_V2(voice_t *const v) { this->voice_V8(v); this->voice_V5(v + 1); this->voice_V2(v + 2); }
void SPC_DSP::voice_V9_V6_V3(voice_t *const v) { this->voice_V9(v); this->voice_V6(v + 1); this->voice_V3(v + 2); }

//// Echo

void SPC_DSP::echo_read(int ch)
{
	int s = static_cast<int16_t>(get_le16(this->ECHO_PTR(ch)));
	// second copy simplifies wrap-around handling
	this->ECHO_FIR(0)[ch] = this->ECHO_FIR(8)[ch] = s >> 1;
}

void SPC_DSP::echo_22()
{
	// History
	if (++this->m.echo_hist_pos >= &this->m.echo_hist[echo_hist_size])
		this->m.echo_hist_pos = this->m.echo_hist;

	this->m.t_echo_ptr = (this->m.t_esa * 0x100 + this->m.echo_offset) & 0xFFFF;
	this->echo_read(0);

	// FIR (using l and r temporaries below helps compiler optimize)
	int l = this->CALC_FIR(0, 0);
	int r = this->CALC_FIR(0, 1);

	this->m.t_echo_in[0] = l;
	this->m.t_echo_in[1] = r;
}
void SPC_DSP::echo_23()
{
	int l = this->CALC_FIR(1, 0) + this->CALC_FIR(2, 0);
	int r = this->CALC_FIR(1, 1) + this->CALC_FIR(2, 1);

	this->m.t_echo_in[0] += l;
	this->m.t_echo_in[1] += r;

	echo_read(1);
}
void SPC_DSP::echo_24()
{
	int l = this->CALC_FIR(3, 0) + this->CALC_FIR(4, 0) + this->CALC_FIR(5, 0);
	int r = this->CALC_FIR(3, 1) + this->CALC_FIR(4, 1) + this->CALC_FIR(5, 1);

	this->m.t_echo_in[0] += l;
	this->m.t_echo_in[1] += r;
}
void SPC_DSP::echo_25()
{
	int l = this->m.t_echo_in[0] + this->CALC_FIR(6, 0);
	int r = this->m.t_echo_in[1] + this->CALC_FIR(6, 1);

	l = static_cast<int16_t>(l);
	r = static_cast<int16_t>(r);

	l += static_cast<int16_t>(this->CALC_FIR(7, 0));
	r += static_cast<int16_t>(this->CALC_FIR(7, 1));

	CLAMP16(l);
	CLAMP16(r);

	this->m.t_echo_in[0] = l & ~1;
	this->m.t_echo_in[1] = r & ~1;
}
int SPC_DSP::echo_output(int ch)
{
	int out = static_cast<int16_t>((this->m.t_main_out [ch] * static_cast<int8_t>(this->m.regs[r_mvoll + ch * 0x10])) >> 7) +
		static_cast<int16_t>((this->m.t_echo_in [ch] * static_cast<int8_t>(this->m.regs[r_evoll + ch * 0x10])) >> 7);
	CLAMP16(out);
	return out;
}
void SPC_DSP::echo_26()
{
	// Left output volumes
	// (save sample for next clock so we can output both together)
	this->m.t_main_out[0] = echo_output(0);

	// Echo feedback
	int l = this->m.t_echo_out[0] + static_cast<int16_t>((this->m.t_echo_in[0] * static_cast<int8_t>(this->m.regs[r_efb])) >> 7);
	int r = this->m.t_echo_out[1] + static_cast<int16_t>((this->m.t_echo_in[1] * static_cast<int8_t>(this->m.regs[r_efb])) >> 7);

	CLAMP16(l);
	CLAMP16(r);

	this->m.t_echo_out[0] = l & ~1;
	this->m.t_echo_out[1] = r & ~1;
}
void SPC_DSP::echo_27()
{
	// Output
	int l = this->m.t_main_out[0];
	int r = echo_output(1);
	this->m.t_main_out[0] = this->m.t_main_out[1] = 0;

	// TODO: global muting isn't this simple (turns DAC on and off
	// or something, causing small ~37-sample pulse when first muted)
	if (this->m.regs[r_flg] & 0x40)
		l = r = 0;

	// Output sample to DAC
#ifdef SPC_DSP_OUT_HOOK
	SPC_DSP_OUT_HOOK(l, r);
#else
	sample_t *out = this->m.out;
	out[0] = l;
	out[1] = r;
	out += 2;
	if (out >= m.out_end)
	{
		out = this->m.extra;
		this->m.out_end = &this->m.extra[extra_size];
	}
	this->m.out = out;
#endif
}
void SPC_DSP::echo_28()
{
	this->m.t_echo_enabled = this->m.regs[r_flg];
}
void SPC_DSP::echo_write(int ch)
{
	if (!(this->m.t_echo_enabled & 0x20))
	{
		if (this->m.t_echo_ptr >= 0xffc0 && this->rom_enabled)
			set_le16(&this->hi_ram[this->m.t_echo_ptr + ch * 2 - 0xffc0], this->m.t_echo_out [ch]);
		else
			set_le16(this->ECHO_PTR(ch), this->m.t_echo_out[ch]);
	}

	this->m.t_echo_out[ch] = 0;
}
void SPC_DSP::echo_29()
{
	this->m.t_esa = this->m.regs[r_esa];

	if (!this->m.echo_offset)
		this->m.echo_length = (this->m.regs[r_edl] & 0x0F) * 0x800;

	this->m.echo_offset += 4;
	if (this->m.echo_offset >= this->m.echo_length)
		this->m.echo_offset = 0;

	// Write left echo
	this->echo_write(0);

	this->m.t_echo_enabled = this->m.regs[r_flg];
}
void SPC_DSP::echo_30()
{
	// Write right echo
	this->echo_write(1);
}

//// Timing

// Execute clock for a particular voice
#define V(clock, voice) voice_##clock(&this->m.voices[voice]);

/* The most common sequence of clocks uses composite operations
for efficiency. For example, the following are equivalent to the
individual steps on the right:

V(V7_V4_V1,2) -> V(V7,2) V(V4,3) V(V1,5)
V(V8_V5_V2,2) -> V(V8,2) V(V5,3) V(V2,4)
V(V9_V6_V3,2) -> V(V9,2) V(V6,3) V(V3,4) */

// Voice      0      1      2      3      4      5      6      7
#define GEN_DSP_TIMING \
PHASE(0) V(V5, 0) V(V2, 1) \
PHASE(1) V(V6, 0) V(V3, 1) \
PHASE(2) V(V7_V4_V1, 0) \
PHASE(3) V(V8_V5_V2, 0) \
PHASE(4) V(V9_V6_V3, 0) \
PHASE(5) V(V7_V4_V1, 1) \
PHASE(6) V(V8_V5_V2, 1) \
PHASE(7) V(V9_V6_V3, 1) \
PHASE(8) V(V7_V4_V1, 2) \
PHASE(9) V(V8_V5_V2, 2) \
PHASE(10) V(V9_V6_V3, 2) \
PHASE(11) V(V7_V4_V1, 3) \
PHASE(12) V(V8_V5_V2, 3) \
PHASE(13) V(V9_V6_V3, 3) \
PHASE(14) V(V7_V4_V1, 4) \
PHASE(15) V(V8_V5_V2, 4) \
PHASE(16) V(V9_V6_V3, 4) \
PHASE(17) V(V1, 0) V(V7, 5) V(V4, 6) \
PHASE(18) V(V8_V5_V2, 5) \
PHASE(19) V(V9_V6_V3, 5) \
PHASE(20) V(V1, 1) V(V7, 6) V(V4, 7) \
PHASE(21) V(V8, 6) V(V5, 7) V(V2, 0) /* t_brr_next_addr order dependency */ \
PHASE(22) V(V3a, 0) V(V9, 6) V(V6, 7) echo_22(); \
PHASE(23) V(V7, 7) echo_23(); \
PHASE(24) V(V8, 7) echo_24(); \
PHASE(25) V(V3b, 0) V(V9, 7) echo_25(); \
PHASE(26) echo_26(); \
PHASE(27) misc_27(); echo_27(); \
PHASE(28) misc_28(); echo_28(); \
PHASE(29) misc_29(); echo_29(); \
PHASE(30) misc_30(); V(V3c, 0) echo_30(); \
PHASE(31) V(V4, 0) V(V1, 2)

#ifndef SPC_DSP_CUSTOM_RUN
void SPC_DSP::run(int clocks_remain)
{
	assert(clocks_remain > 0);

	int phase = this->m.phase;
	this->m.phase = (phase + clocks_remain) & 31;
	switch (phase)
	{
		loop:
#define PHASE(n) if (n && !--clocks_remain) break; case n:
		GEN_DSP_TIMING
#undef PHASE

		if (--clocks_remain)
			goto loop;
	}
}
#endif

//// Setup

void SPC_DSP::init(uint8_t *ram_64k)
{
	this->m.ram = ram_64k;
	this->mute_voices(0);
	this->disable_surround(false);
	this->set_output(nullptr, 0);
	this->reset();

	this->stereo_switch = 0xffff;

#ifndef NDEBUG
	// be sure this sign-extends
	assert(static_cast<int16_t>(0x8000) == -0x8000);

	// be sure right shift preserves sign
	assert((-1 >> 1) == -1);

	// check clamp macro
	int i = 0x8000;
	CLAMP16(i);
	assert(i == 0x7FFF);

	i = -0x8001;
	CLAMP16(i);
	assert(i == -0x8000);

	blargg_verify_byte_order();
#endif
}

void SPC_DSP::soft_reset_common()
{
	assert(this->m.ram); // init() must have been called already

	this->m.noise = 0x4000;
	this->m.echo_hist_pos = this->m.echo_hist;
	this->m.every_other_sample = true;
	this->m.echo_offset = 0;
	this->m.phase = 0;

	this->init_counter();

	for (int i = 0; i < voice_count; ++i)
		this->m.voices[i].voice_number = i;
}

void SPC_DSP::soft_reset()
{
	this->m.regs[r_flg] = 0xE0;
	this->soft_reset_common();
}

void SPC_DSP::load(const uint8_t regs[register_count])
{
	std::copy_n(&regs[0], static_cast<int>(register_count), &this->m.regs[0]);
	memset(&this->m.regs[register_count], 0, offsetof(state_t, ram) - register_count);

	// Internal state
	for (int i = voice_count; --i >= 0;)
	{
		auto &v = this->m.voices[i];
		v.brr_offset = 1;
		v.vbit = 1 << i;
		v.regs = &this->m.regs[i * 0x10];
	}
	this->m.new_kon = this->m.regs[r_kon];
	this->m.t_dir = this->m.regs[r_dir];
	this->m.t_esa = this->m.regs[r_esa];

	this->soft_reset_common();
}

void SPC_DSP::reset()
{
	this->load(initial_regs);
}

//// Snes9x Accessor

void SPC_DSP::set_stereo_switch(int value)
{
	this->stereo_switch = value;
}

uint8_t SPC_DSP::reg_value(int ch, int addr)
{
	return this->m.voices[ch].regs[addr];
}

int SPC_DSP::envx_value(int ch)
{
	return this->m.voices[ch].env;
}
