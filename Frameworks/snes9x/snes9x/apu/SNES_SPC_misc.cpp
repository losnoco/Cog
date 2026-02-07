// SPC emulation support: init, sample buffering, reset, SPC loading

// snes_spc 0.9.0. http://www.slack.net/~ant/

#include <algorithm>
#include <cstring>
#include "SNES_SPC.h"

/* Copyright (C) 2004-2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

//// Init

void SNES_SPC::init()
{
	memset(&this->m, 0, sizeof(this->m));
	this->dsp.init(this->m.ram.ram);

	this->m.tempo = tempo_unit;

	// Most SPC music doesn't need ROM, and almost all the rest only rely
	// on these two bytes
	this->m.rom[0x3E] = 0xFF;
	this->m.rom[0x3F] = 0xC0;

	static const unsigned char cycle_table[] =
	{
		//01    23    45    67    89    AB    CD    EF
		0x28, 0x47, 0x34, 0x36, 0x26, 0x54, 0x54, 0x68, // 0
		0x48, 0x47, 0x45, 0x56, 0x55, 0x65, 0x22, 0x46, // 1
		0x28, 0x47, 0x34, 0x36, 0x26, 0x54, 0x54, 0x74, // 2
		0x48, 0x47, 0x45, 0x56, 0x55, 0x65, 0x22, 0x38, // 3
		0x28, 0x47, 0x34, 0x36, 0x26, 0x44, 0x54, 0x66, // 4
		0x48, 0x47, 0x45, 0x56, 0x55, 0x45, 0x22, 0x43, // 5
		0x28, 0x47, 0x34, 0x36, 0x26, 0x44, 0x54, 0x75, // 6
		0x48, 0x47, 0x45, 0x56, 0x55, 0x55, 0x22, 0x36, // 7
		0x28, 0x47, 0x34, 0x36, 0x26, 0x54, 0x52, 0x45, // 8
		0x48, 0x47, 0x45, 0x56, 0x55, 0x55, 0x22, 0xC5, // 9
		0x38, 0x47, 0x34, 0x36, 0x26, 0x44, 0x52, 0x44, // A
		0x48, 0x47, 0x45, 0x56, 0x55, 0x55, 0x22, 0x34, // B
		0x38, 0x47, 0x45, 0x47, 0x25, 0x64, 0x52, 0x49, // C
		0x48, 0x47, 0x56, 0x67, 0x45, 0x55, 0x22, 0x83, // D
		0x28, 0x47, 0x34, 0x36, 0x24, 0x53, 0x43, 0x40, // E
		0x48, 0x47, 0x45, 0x56, 0x34, 0x54, 0x22, 0x60  // F
	};

	// unpack cycle table
	for (int i = 0; i < 128; ++i)
	{
		int n = cycle_table[i];
		this->m.cycle_table[i * 2] = n >> 4;
		this->m.cycle_table[i * 2 + 1] = n & 0x0F;
	}

	this->allow_time_overflow = false;

	this->dsp.rom = this->m.rom;
	this->dsp.hi_ram = this->m.hi_ram;

	this->reset();
}

void SNES_SPC::init_rom(const uint8_t in[rom_size])
{
	std::copy_n(&in[0], static_cast<int>(rom_size), &this->m.rom[0]);
}

void SNES_SPC::set_tempo(int t)
{
	this->m.tempo = t;
	static const int timer2_shift = 4; // 64 kHz
	static const int other_shift = 3; //  8 kHz

#ifdef SPC_DISABLE_TEMPO
	this->m.timers[2].prescaler = timer2_shift;
	this->m.timers[1].prescaler = timer2_shift + other_shift;
	this->m.timers[0].prescaler = timer2_shift + other_shift;
#else
	if (!t)
		t = 1;
	static const int timer2_rate = 1 << timer2_shift;
	int rate = (timer2_rate * tempo_unit + (t >> 1)) / t;
	if (rate < timer2_rate / 4)
		rate = timer2_rate / 4; // max 4x tempo
	this->m.timers[2].prescaler = rate;
	this->m.timers[1].prescaler = rate << other_shift;
	this->m.timers[0].prescaler = rate << other_shift;
#endif
}

// Timer registers have been loaded. Applies these to the timers. Does not
// reset timer prescalers or dividers.
void SNES_SPC::timers_loaded()
{
	for (int i = 0; i < timer_count; ++i)
	{
		auto &t = this->m.timers[i];
		t.period = IF_0_THEN_256(this->m.smp_regs[0][r_t0target + i]);
		t.enabled = !!((this->m.smp_regs[0][r_control] >> i) & 1);
		t.counter = this->m.smp_regs[1][r_t0out + i] & 0x0F;
	}

	this->set_tempo(this->m.tempo);
}

// Loads registers from unified 16-byte format
void SNES_SPC::load_regs(const uint8_t in[reg_count])
{
	std::copy_n(&in[0], static_cast<int>(reg_count), &this->m.smp_regs[0][0]);
	std::copy_n(&this->m.smp_regs[0][0], static_cast<int>(reg_count), &this->m.smp_regs[1][0]);

	// These always read back as 0
	this->m.smp_regs[1][r_test] = this->m.smp_regs[1][r_control] = this->m.smp_regs[1][r_t0target] = this->m.smp_regs[1][r_t1target] = this->m.smp_regs[1][r_t2target] = 0;
}

// RAM was just loaded from SPC, with $F0-$FF containing SMP registers
// and timer counts. Copies these to proper registers.
void SNES_SPC::ram_loaded()
{
	this->m.rom_enabled = this->dsp.rom_enabled = false;
	this->load_regs(&this->m.ram.ram[0xF0]);

	// Put STOP instruction around memory to catch PC underflow/overflow
	memset(this->m.ram.padding1, cpu_pad_fill, sizeof(this->m.ram.padding1));
	memset(this->m.ram.padding2, cpu_pad_fill, sizeof(this->m.ram.padding2));
}

// Registers were just loaded. Applies these new values.
void SNES_SPC::regs_loaded()
{
	this->enable_rom(!!(this->m.smp_regs[0][r_control] & 0x80));
	this->timers_loaded();
}

void SNES_SPC::reset_time_regs()
{
	this->m.spc_time = 0;
	this->m.dsp_time = 0;

	std::for_each(&this->m.timers[0], &this->m.timers[timer_count], [](Timer &t)
	{
		t.next_time = 1;
		t.divider = 0;
	});

	this->regs_loaded();

	this->m.extra_clocks = 0;
	this->reset_buf();
}

void SNES_SPC::reset_common(int timer_counter_init)
{
	std::fill_n(&this->m.smp_regs[1][r_t0out], static_cast<int>(timer_count), timer_counter_init);

	// Run IPL ROM
	memset(&this->m.cpu_regs, 0, sizeof(this->m.cpu_regs));
	this->m.cpu_regs.pc = rom_addr;

	this->m.smp_regs[0][r_test] = 0x0A;
	this->m.smp_regs[0][r_control] = 0xB0; // ROM enabled, clear ports
	std::fill_n(&this->m.smp_regs[1][r_cpuio0], static_cast<int>(port_count), 0);

	this->reset_time_regs();
}

void SNES_SPC::soft_reset()
{
	this->reset_common(0);
	this->dsp.soft_reset();
}

void SNES_SPC::reset()
{
	this->m.cpu_regs.pc = 0xFFC0;
	this->m.cpu_regs.a = this->m.cpu_regs.x = this->m.cpu_regs.y = 0x00;
	this->m.cpu_regs.psw = 0x02;
	this->m.cpu_regs.sp = 0xEF;
	std::fill_n(&this->m.ram.ram[0], 0x10000, 0);
	this->ram_loaded();
	this->reset_common(0x0F);
	this->dsp.reset();
}

void SNES_SPC::clear_echo()
{
	if (!(this->dsp.read(SPC_DSP::r_flg) & 0x20))
	{
		int addr = 0x100 * this->dsp.read(SPC_DSP::r_esa);
		int end  = addr + 0x800 * (this->dsp.read(SPC_DSP::r_edl) & 0x0F);
		if (end > 0x10000)
			end = 0x10000;
		std::fill(&this->m.ram.ram[addr], &this->m.ram.ram[end], 0xFF);
	}
}

//// Sample output

void SNES_SPC::reset_buf()
{
	// Start with half extra buffer of silence
	sample_t *out = this->m.extra_buf;
	while (out < &this->m.extra_buf[extra_size / 2])
		*out++ = 0;

	this->m.extra_pos = out;
	this->m.buf_begin = nullptr;

	this->dsp.set_output(nullptr, 0);
}

void SNES_SPC::set_output(sample_t *out, int size)
{
	assert(!(size & 1)); // size must be even

	this->m.extra_clocks &= clocks_per_sample - 1;
	if (out)
	{
		auto out_end = out + size;
		this->m.buf_begin = out;
		this->m.buf_end = out_end;

		// Copy extra to output
		auto in = this->m.extra_buf;
		while (in < this->m.extra_pos && out < out_end)
			*out++ = *in++;

		// Handle output being full already
		if (out >= out_end)
		{
			// Have DSP write to remaining extra space
			out = this->dsp.extra();
			out_end = &this->dsp.extra()[extra_size];

			// Copy any remaining extra samples as if DSP wrote them
			while (in < m.extra_pos)
				*out++ = *in++;
			assert(out <= out_end);
		}

		this->dsp.set_output(out, out_end - out);
	}
	else
		this->reset_buf();
}

void SNES_SPC::save_extra()
{
	// Get end pointers
	auto main_end = this->m.buf_end; // end of data written to buf
	auto dsp_end = this->dsp.out_pos(); // end of data written to dsp.extra()
	if (this->m.buf_begin <= dsp_end && dsp_end <= main_end)
	{
		main_end = dsp_end;
		dsp_end = this->dsp.extra(); // nothing in DSP's extra
	}

	// Copy any extra samples at these ends into extra_buf
	auto out = this->m.extra_buf;
	for (auto in = this->m.buf_begin + this->sample_count(); in < main_end; ++in)
		*out++ = *in;
	for (auto in = this->dsp.extra(); in < dsp_end; ++in)
		*out++ = *in;

	this->m.extra_pos = out;
	assert(out <= &this->m.extra_buf[extra_size]);
}

void SNES_SPC::play(int count, sample_t *out)
{
	assert(!(count & 1)); // must be even
	if (count)
	{
		this->set_output(out, count);
		this->end_frame(count * (clocks_per_sample / 2));
	}
}

void SNES_SPC::skip(int count)
{
	this->play(count, nullptr);
}

//// Snes9x Accessor

void SNES_SPC::dsp_set_stereo_switch(int value)
{
	this->dsp.set_stereo_switch(value);
}

uint8_t SNES_SPC::dsp_reg_value(int ch, int addr)
{
	return this->dsp.reg_value(ch, addr);
}

int SNES_SPC::dsp_envx_value(int ch)
{
	return this->dsp.envx_value(ch);
}
