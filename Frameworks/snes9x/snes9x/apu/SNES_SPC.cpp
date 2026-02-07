// Core SPC emulation: CPU, timers, SMP registers, memory

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

//// Timers

#ifdef SPC_DISABLE_TEMPO
template<typename T> static inline T TIMER_DIV(SNES_SPC::Timer *t, const T &n) { return n >> t->prescaler; }
template<typename T> static inline T TIMER_MUL(SNES_SPC::Timer *t, const T &n) { return n << t->prescaler; }
#else
template<typename T> static inline T TIMER_DIV(SNES_SPC::Timer *t, const T &n) { return n / t->prescaler; }
template<typename T> static inline T TIMER_MUL(SNES_SPC::Timer *t, const T &n) { return n * t->prescaler; }
#endif

auto SNES_SPC::run_timer_(Timer *t, rel_time_t time) -> Timer *
{
	int elapsed = TIMER_DIV(t, time - t->next_time) + 1;
	t->next_time += TIMER_MUL(t, elapsed);

	if (t->enabled)
	{
		int remain = IF_0_THEN_256(t->period - t->divider);
		int divider = t->divider + elapsed;
		int over = elapsed - remain;
		if (over >= 0)
		{
			int n = over / t->period;
			t->counter = (t->counter + 1 + n) & 0x0F;
			divider = over - n * t->period;
		}
		t->divider = static_cast<uint8_t>(divider);
	}
	return t;
}

auto SNES_SPC::run_timer(Timer *t, rel_time_t time) -> Timer *
{
	if (time >= t->next_time)
		t = this->run_timer_(t, time);
	return t;
}

//// ROM

void SNES_SPC::enable_rom(bool enable)
{
	if (this->m.rom_enabled != enable)
	{
		this->m.rom_enabled = this->dsp.rom_enabled = enable;
		if (enable)
			std::copy_n(&this->m.ram.ram[rom_addr], static_cast<int>(rom_size), &this->m.hi_ram[0]);
		auto data = enable ? &this->m.rom[0] : &this->m.hi_ram[0];
		std::copy_n(&data[0], static_cast<int>(rom_size), &this->m.ram.ram[rom_addr]);
		// TODO: ROM can still get overwritten when DSP writes to echo buffer
	}
}

//// DSP

void SNES_SPC::RUN_DSP(rel_time_t time)
{
	int count = time - this->m.dsp_time;
	if (count)
	{
		assert(count > 0);
		this->m.dsp_time = time;
		this->dsp.run(count);
	}
}

int SNES_SPC::dsp_read(rel_time_t time)
{
	this->RUN_DSP(time);

	int result = this->dsp.read(this->m.smp_regs[0][r_dspaddr] & 0x7F);

#ifdef SPC_DSP_READ_HOOK
	SPC_DSP_READ_HOOK(spc_time + time, this->m.smp_regs[0][r_dspaddr] & 0x7F, result);
#endif

	return result;
}

void SNES_SPC::dsp_write(int data, rel_time_t time)
{
	this->RUN_DSP(time);

#ifdef SPC_DSP_WRITE_HOOK
	SPC_DSP_WRITE_HOOK(this->m.spc_time + time, this->m.smp_regs[0][r_dspaddr], static_cast<uint8_t>(data));
#endif

	if (this->m.smp_regs[0][r_dspaddr] <= 0x7F)
		this->dsp.write(this->m.smp_regs[0][r_dspaddr], data);
}

//// CPU write

// divided into multiple functions to keep rarely-used functionality separate
// so often-used functionality can be optimized better by compiler

// If write isn't preceded by read, data has this added to it
static const int no_read_before_write = 0x2000;

void SNES_SPC::cpu_write_smp_reg_(int data, rel_time_t time, int addr)
{
	switch (addr)
	{
		case r_t0target:
		case r_t1target:
		case r_t2target:
		{
			auto t = &this->m.timers[addr - r_t0target];
			int period = IF_0_THEN_256(data);
			if (t->period != period)
			{
				t = this->run_timer(t, time);
				t->period = period;
			}
			break;
		}

		case r_t0out:
		case r_t1out:
		case r_t2out:
			if (data < no_read_before_write  / 2)
				this->run_timer(&this->m.timers[addr - r_t0out], time - 1)->counter = 0;
			break;

		// Registers that act like RAM
		case 0x8:
		case 0x9:
			this->m.smp_regs[1][addr] = static_cast<uint8_t>(data);
			break;

		case r_test:
			break;

		case r_control:
			// port clears
			if (data & 0x10)
			{
				this->m.smp_regs[1][r_cpuio0] = 0;
				this->m.smp_regs[1][r_cpuio1] = 0;
			}
			if (data & 0x20)
			{
				this->m.smp_regs[1][r_cpuio2] = 0;
				this->m.smp_regs[1][r_cpuio3] = 0;
			}

			// timers
			for (int i = 0; i < timer_count; ++i)
			{
				auto t = &this->m.timers[i];
				bool enabled = !!((data >> i) & 1);
				if (t->enabled != enabled)
				{
					t = this->run_timer(t, time);
					t->enabled = enabled;
					if (enabled)
						t->divider = t->counter = 0;
				}
			}
			this->enable_rom(!!(data & 0x80));
	}
}

void SNES_SPC::cpu_write_smp_reg(int data, rel_time_t time, int addr)
{
	if (addr == r_dspdata) // 99%
		this->dsp_write(data, time);
	else
		this->cpu_write_smp_reg_(data, time, addr);
}

void SNES_SPC::cpu_write_high(int data, int i, rel_time_t time)
{
	if (i < rom_size)
	{
		this->m.hi_ram [i] = static_cast<uint8_t>(data);
		if (this->m.rom_enabled)
			this->m.ram.ram[i + rom_addr] = this->m.rom[i]; // restore overwritten ROM
	}
	else
	{
		assert(this->m.ram.ram[i + rom_addr] == static_cast<uint8_t>(data));
		this->m.ram.ram[i + rom_addr] = cpu_pad_fill; // restore overwritten padding
		this->cpu_write(data, i + rom_addr - 0x10000, time);
	}
}

static const int bits_in_int = CHAR_BIT * sizeof(int);

void SNES_SPC::cpu_write(int data, int addr, rel_time_t time)
{
	// RAM
	this->m.ram.ram[addr] = static_cast<uint8_t>(data);
	int reg = addr - 0xF0;
	if (reg >= 0) // 64%
	{
		// $F0-$FF
		if (reg < reg_count) // 87%
		{
			this->m.smp_regs[0][reg] = static_cast<uint8_t>(data);

			// Ports
#ifdef SPC_PORT_WRITE_HOOK
			if (static_cast<unsigned>(reg - r_cpuio0) < port_count)
				SPC_PORT_WRITE_HOOK(this->m.spc_time + time, (reg - r_cpuio0), static_cast<uint8_t>(data), &this->m.smp_regs[0][r_cpuio0]);
#endif

			// Registers other than $F2 and $F4-$F7
			//if ( reg != 2 && reg != 4 && reg != 5 && reg != 6 && reg != 7 )
			// TODO: this is a bit on the fragile side
			if (((~0x2F00 << (bits_in_int - 16)) << reg) < 0) // 36%
				this->cpu_write_smp_reg(data, time, reg);
		}
		// High mem/address wrap-around
		else
		{
			reg -= rom_addr - 0xF0;
			if (reg >= 0) // 1% in IPL ROM area or address wrapped around
				this->cpu_write_high(data, reg, time);
		}
	}
}

//// CPU read

int SNES_SPC::cpu_read_smp_reg(int reg, rel_time_t time)
{
	int result = this->m.smp_regs[1][reg];
	reg -= r_dspaddr;
	// DSP addr and data
	if (static_cast<unsigned>(reg) <= 1) // 4% 0xF2 and 0xF3
	{
		result = this->m.smp_regs[0][r_dspaddr];
		if (static_cast<unsigned>(reg) == 1)
			result = this->dsp_read(time); // 0xF3
	}
	return result;
}

int SNES_SPC::cpu_read(int addr, rel_time_t time)
{
	// RAM
	int result = this->m.ram.ram[addr];
	int reg = addr - 0xF0;
	if (reg >= 0) // 40%
	{
		reg -= 0x10;
		if (static_cast<unsigned>(reg) >= 0xFF00) // 21%
		{
			reg += 0x10 - r_t0out;

			// Timers
			if (static_cast<unsigned>(reg) < timer_count) // 90%
			{
				auto t = &this->m.timers[reg];
				if (time >= t->next_time)
					t = this->run_timer_(t, time);
				result = t->counter;
				t->counter = 0;
			}
			// Other registers
			else if (reg < 0) // 10%
				result = this->cpu_read_smp_reg(reg + r_t0out, time);
			else // 1%
			{
				assert(reg + (r_t0out + 0xF0 - 0x10000) < 0x100);
				result = this->cpu_read(reg + (r_t0out + 0xF0 - 0x10000), time);
			}
		}
	}

	return result;
}

//// Run

static const int cpu_lag_max = 12 - 1; // DIV YA,X takes 12 clocks

void SNES_SPC::end_frame(time_t end_time)
{
	// Catch CPU up to as close to end as possible. If final instruction
	// would exceed end, does NOT execute it and leaves m.spc_time < end.
	if (end_time > this->m.spc_time)
		this->run_until_(end_time);

	this->m.spc_time -= end_time;
	this->m.extra_clocks += end_time;

	// Greatest number of clocks early that emulation can stop early due to
	// not being able to execute current instruction without going over
	// allowed time.
	assert(-cpu_lag_max <= this->m.spc_time && this->m.spc_time <= cpu_lag_max);

	// Catch timers up to CPU
	for (int i = 0; i < timer_count; ++i)
		this->run_timer(&this->m.timers [i], 0);

	// Catch DSP up to CPU
	if (this->m.dsp_time < 0)
		this->RUN_DSP(0);

	// Save any extra samples beyond what should be generated
	if (this->m.buf_begin)
		this->save_extra();
}

// Inclusion here allows static memory access functions and better optimization
#include "SPC_CPU.h"
