// snes_spc 0.9.0. http://www.slack.net/~ant/

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

#pragma once

//// Memory access

// TODO: remove non-wrapping versions?
#define SPC_NO_SP_WRAPAROUND 0

unsigned SNES_SPC::CPU_mem_bit(const uint8_t *pc, rel_time_t rel_time)
{
	unsigned addr = get_le16(pc);
	unsigned t = this->cpu_read(addr & 0x1FFF, rel_time) >> (addr >> 13);
	return (t << 8) & 0x100;
}

//// Status flag handling

// Hex value in name to clarify code and bit shifting.
// Flag stored in indicated variable during emulation
const int n80 = 0x80; // nz
const int v40 = 0x40; // psw
const int p20 = 0x20; // dp
const int b10 = 0x10; // psw
const int h08 = 0x08; // psw
const int i04 = 0x04; // psw
const int z02 = 0x02; // nz
const int c01 = 0x01; // c

const int nz_neg_mask = 0x880; // either bit set indicates N flag set

uint8_t *SNES_SPC::run_until_(time_t end_time)
{
	rel_time_t rel_time = this->m.spc_time - end_time;
	/*assert( rel_time <= 0 );*/
	this->m.spc_time = end_time;
	this->m.dsp_time += rel_time;
	this->m.timers[0].next_time += rel_time;
	this->m.timers[1].next_time += rel_time;
	this->m.timers[2].next_time += rel_time;

	auto ram = this->m.ram.ram;
	int a = this->m.cpu_regs.a;
	int x = this->m.cpu_regs.x;
	int y = this->m.cpu_regs.y;
	const uint8_t *pc;
	uint8_t *sp;
	int psw;
	int c;
	int nz;
	int dp;

	// timers are by far the most common thing read from dp
	auto CPU_READ_TIMER = [&](rel_time_t offset, int addr_) -> int
	{
		int out;
		rel_time_t adj_time = rel_time + offset;
		int dp_addr = addr_;
		int ti = dp_addr - (r_t0out + 0xF0);
		if (static_cast<unsigned>(ti) < timer_count)
		{
			auto t = &this->m.timers[ti];
			if (adj_time >= t->next_time)
				t = this->run_timer_(t, adj_time);
			out = t->counter;
			t->counter = 0;
		}
		else
		{
			out = ram[dp_addr];
			int i = dp_addr - 0xF0;
			if (static_cast<unsigned>(i) < 0x10)
				out = this->cpu_read_smp_reg(i, adj_time);
		}
		return out;
	};

	auto DP_ADDR = [&](int addr) { return dp + addr; };

	auto READ_PROG16 = [&](int addr) { return get_le16(ram + addr); };

	auto SET_PC = [&](int n) { pc = ram + n; };
	auto GET_PC = [&]() { return pc - ram; };

	auto SET_SP = [&](int v) { sp = ram + 0x101 + v; };
	auto GET_SP = [&]() { return sp - 0x101 - ram; };

	auto PUSH16 = [&](int data)
	{
		sp -= 2;
#if !SPC_NO_SP_WRAPAROUND
		int addr = sp - ram;
		if (addr > 0x100)
#endif
			set_le16(sp, data);
#if !SPC_NO_SP_WRAPAROUND
		else
		{
			ram[static_cast<uint8_t>(addr) + 0x100] = static_cast<uint8_t>(data);
			sp[1] = static_cast<uint8_t>(data >> 8);
			sp += 0x100;
		}
#endif
	};
	auto PUSH = [&](int data)
	{
		*--sp = static_cast<uint8_t>(data);
#if !SPC_NO_SP_WRAPAROUND
		if (sp - ram == 0x100)
			sp += 0x100;
#endif
	};
	auto POP = [&](int &out)
	{
		out = *sp++;
#if !SPC_NO_SP_WRAPAROUND
		if (sp - ram == 0x201)
		{
			out = sp[-0x101];
			sp -= 0x100;
		}
#endif
	};

	auto MEM_BIT = [&](rel_time_t rel) { return this->CPU_mem_bit(pc, rel_time + rel); };

	auto GET_PSW = [&](int &out)
	{
		out = psw & ~(n80 | p20 | z02 | c01);
		out |= (c >> 8) & c01;
		out |= (dp >> 3) & p20;
		out |= ((nz >> 4) | nz) & n80;
		if (!static_cast<uint8_t>(nz))
			out |= z02;
	};
	auto SET_PSW = [&](int in)
	{
		psw = in;
		c = in << 8;
		dp = (in << 3) & 0x100;
		nz = ((in << 4) & 0x800) | (~in & z02);
	};

	SET_PC(this->m.cpu_regs.pc);
	SET_SP(this->m.cpu_regs.sp);
	SET_PSW(this->m.cpu_regs.psw);

	goto loop;

	// Main loop

cbranch_taken_loop:
	pc += *reinterpret_cast<const int8_t *>(pc);
inc_pc_loop:
	++pc;
loop:
	unsigned data;

	unsigned opcode = *pc;
	if (this->allow_time_overflow && rel_time >= 0)
		goto stop;
	if ((rel_time += this->m.cycle_table[opcode]) > 0 && !this->allow_time_overflow)
		goto out_of_time;

#ifdef SPC_CPU_OPCODE_HOOK
	SPC_CPU_OPCODE_HOOK(GET_PC(), opcode);
#endif

	// TODO: if PC is at end of memory, this will get wrong operand (very obscure)
	data = *++pc;
	switch (opcode)
	{
		// Common instructions

#define BRANCH(cond) \
{ \
	++pc; \
	pc += static_cast<int8_t>(data); \
	if (cond) \
		goto loop; \
	pc -= static_cast<int8_t>(data); \
	rel_time -= 2; \
	goto loop; \
}

		case 0xF0: // BEQ
			BRANCH(!static_cast<uint8_t>(nz)) // 89% taken

		case 0xD0: // BNE
			BRANCH(static_cast<uint8_t>(nz))

		case 0x3F: // CALL
		{
			int old_addr = GET_PC() + 2;
			SET_PC(get_le16(pc));
			PUSH16(old_addr);
			goto loop;
		}

		case 0x6F:// RET
#if SPC_NO_SP_WRAPAROUND
			SET_PC(get_le16(sp));
			sp += 2;
#else
			{
				int addr = sp - ram;
				SET_PC(get_le16(sp));
				sp += 2;
				if (addr < 0x1FF)
					goto loop;

				SET_PC(sp[-0x101] * 0x100 + ram[static_cast<uint8_t>(addr) + 0x100]);
				sp -= 0x100;
			}
#endif
			goto loop;

		case 0xE4: // MOV a,dp
			++pc;
			// 80% from timer
			a = nz = CPU_READ_TIMER(0, DP_ADDR(data));
			goto loop;

		case 0xFA: // MOV dp,dp
		{
			int temp = CPU_READ_TIMER(-2, DP_ADDR(data));
			data = temp + no_read_before_write;
		}
		// fall through
		case 0x8F: // MOV dp,#imm
		{
			int temp = *(pc + 1);
			pc += 2;
			{
				int i = dp + temp;
				ram[i] = static_cast<uint8_t>(data);
				i -= 0xF0;
				if (static_cast<unsigned>(i) < 0x10) // 76%
				{
					this->m.smp_regs[0][i] = static_cast<uint8_t>(data);

					// Registers other than $F2 and $F4-$F7
					//if ( i != 2 && i != 4 && i != 5 && i != 6 && i != 7 )
					if (((~0x2F00 << (bits_in_int - 16)) << i) < 0) // 12%
						this->cpu_write_smp_reg(data, rel_time, i);
				}
			}
			goto loop;
		}

		case 0xC4: // MOV dp,a
			++pc;
			{
				int i = dp + data;
				ram[i] = static_cast<uint8_t>(a);
				i -= 0xF0;
				if (static_cast<unsigned>(i) < 0x10) // 39%
				{
					unsigned sel = i - 2;
					this->m.smp_regs[0][i] = static_cast<uint8_t>(a);

					if (sel == 1) // 51% $F3
						this->dsp_write(a, rel_time);
					else if (sel > 1) // 1% not $F2 or $F3
						this->cpu_write_smp_reg_(a, rel_time, i);
				}
			}
			goto loop;

#define CASE(n) case n:

// Define common address modes based on opcode for immediate mode. Execution
// ends with data set to the address of the operand.
#define ADDR_MODES_(op) \
	CASE(op - 0x02) /* (X) */ \
		data = x + dp; \
		--pc; \
		goto end_##op; \
	CASE(op + 0x0F) /* (dp)+Y */ \
		data = READ_PROG16(data + dp) + y; \
		goto end_##op; \
	CASE(op - 0x01) /* (dp+X) */ \
		data = READ_PROG16(static_cast<uint8_t>(data + x) + dp); \
		goto end_##op; \
	CASE(op + 0x0E) /* abs+Y */ \
		data += y; \
		goto abs_##op; \
	CASE(op + 0x0D) /* abs+X */ \
		data += x; \
	CASE(op - 0x03) /* abs */ \
	abs_##op: \
		data += 0x100 * *(++pc); \
		goto end_##op; \
	CASE(op + 0x0C) /* dp+X */ \
		data = static_cast<uint8_t>(data + x);

#define ADDR_MODES_NO_DP(op) \
	ADDR_MODES_(op) \
		data += dp; \
	end_##op:

#define ADDR_MODES(op) \
	ADDR_MODES_(op) \
	CASE(op - 0x04) /* dp */ \
		data += dp; \
	end_##op:

		// 1. 8-bit Data Transmission Commands. Group I

		ADDR_MODES_NO_DP(0xE8) // MOV A,addr
			a = nz = this->cpu_read(data, rel_time);
			goto inc_pc_loop;

		case 0xBF: // MOV A,(X)+
		{
			int temp = x + dp;
			x = static_cast<uint8_t>(x + 1);
			a = nz = this->cpu_read(temp, rel_time - 1);
			goto loop;
		}

		case 0xE8: // MOV A,imm
			a  = data;
			nz = data;
			goto inc_pc_loop;

		case 0xF9: // MOV X,dp+Y
			data = static_cast<uint8_t>(data + y);
		case 0xF8: // MOV X,dp
			x = nz = CPU_READ_TIMER(0, DP_ADDR(data));
			goto inc_pc_loop;

		case 0xE9: // MOV X,abs
			data = get_le16(pc);
			++pc;
			data = this->cpu_read(data, rel_time);
		case 0xCD: // MOV X,imm
			x  = data;
			nz = data;
			goto inc_pc_loop;

		case 0xFB: // MOV Y,dp+X
			data = static_cast<uint8_t>(data + x);
		case 0xEB: // MOV Y,dp
			// 70% from timer
			++pc;
			y = nz = CPU_READ_TIMER(0, DP_ADDR(data));
			goto loop;

		case 0xEC: // MOV Y,abs
		{
			int temp = get_le16(pc);
			pc += 2;
			y = nz = CPU_READ_TIMER(0, temp);
			//y = nz = this->cpu_read(temp, rel_time);
			goto loop;
		}

		case 0x8D: // MOV Y,imm
			y  = data;
			nz = data;
			goto inc_pc_loop;

		// 2. 8-BIT DATA TRANSMISSION COMMANDS, GROUP 2

		ADDR_MODES_NO_DP(0xC8) // MOV addr,A
			this->cpu_write(a, data, rel_time);
			goto inc_pc_loop;

		{
			int temp;
			case 0xCC: // MOV abs,Y
				temp = y;
				goto mov_abs_temp;
			case 0xC9: // MOV abs,X
				temp = x;
			mov_abs_temp:
				this->cpu_write(temp, get_le16(pc), rel_time);
				pc += 2;
				goto loop;
		}

		case 0xD9: // MOV dp+Y,X
			data = static_cast<uint8_t>(data + y);
		case 0xD8: // MOV dp,X
			this->cpu_write(x, data + dp, rel_time);
			goto inc_pc_loop;

		case 0xDB: // MOV dp+X,Y
			data = static_cast<uint8_t>(data + x);
		case 0xCB: // MOV dp,Y
			this->cpu_write(y, data + dp, rel_time);
			goto inc_pc_loop;

		// 3. 8-BIT DATA TRANSMISSIN COMMANDS, GROUP 3.

		case 0x7D: // MOV A,X
			a = nz = x;
			goto loop;

		case 0xDD: // MOV A,Y
			a = nz = y;
			goto loop;

		case 0x5D: // MOV X,A
			x = nz = a;
			goto loop;

		case 0xFD: // MOV Y,A
			y = nz = a;
			goto loop;

		case 0x9D: // MOV X,SP
			x = nz = GET_SP();
			goto loop;

		case 0xBD: // MOV SP,X
			SET_SP(x);
			goto loop;

		//case 0xC6: // MOV (X),A (handled by MOV addr,A in group 2)

		case 0xAF: // MOV (X)+,A
			this->cpu_write(a + no_read_before_write, DP_ADDR(x), rel_time);
			++x;
			goto loop;

		// 5. 8-BIT LOGIC OPERATION COMMANDS

#define LOGICAL_OP(op, func) \
	ADDR_MODES(op) /* addr */ \
		data = this->cpu_read(data, rel_time); \
	case op: /* imm */ \
		nz = a func##= data; \
		goto inc_pc_loop; \
		{ \
			unsigned addr; \
			case op + 0x11: /* X,Y */ \
				data = this->cpu_read(DP_ADDR(y), rel_time - 2); \
				addr = x + dp; \
				goto addr_##op; \
			case op + 0x01: /* dp,dp */ \
				data = this->cpu_read(DP_ADDR(data), rel_time - 3); \
			case op + 0x10: /*dp,imm*/ \
			{ \
				auto addr2 = pc + 1; \
				pc += 2; \
				addr = *addr2 + dp; \
			} \
			addr_##op: \
				nz = data func this->cpu_read(addr, rel_time - 1); \
				this->cpu_write(nz, addr, rel_time); \
				goto loop; \
		}

		LOGICAL_OP(0x28, &); // AND

		LOGICAL_OP(0x08, |); // OR

		LOGICAL_OP(0x48, ^); // EOR

		// 4. 8-BIT ARITHMETIC OPERATION COMMANDS

		ADDR_MODES(0x68) // CMP addr
			data = this->cpu_read(data, rel_time);
		case 0x68: // CMP imm
			nz = a - data;
			c = ~nz;
			nz &= 0xFF;
			goto inc_pc_loop;

		case 0x79: // CMP (X),(Y)
			data = this->cpu_read(DP_ADDR(y), rel_time - 2);
			nz = this->cpu_read(DP_ADDR(x), rel_time - 1) - data;
			c = ~nz;
			nz &= 0xFF;
			goto loop;

		case 0x69: // CMP dp,dp
			data = this->cpu_read(DP_ADDR(data), rel_time - 3);
		case 0x78: // CMP dp,imm
			nz = this->cpu_read(DP_ADDR(*(++pc)), rel_time - 1) - data;
			c = ~nz;
			nz &= 0xFF;
			goto inc_pc_loop;

		case 0x3E: // CMP X,dp
			data += dp;
			goto cmp_x_addr;
		case 0x1E: // CMP X,abs
			data = get_le16(pc);
			++pc;
		cmp_x_addr:
			data = this->cpu_read(data, rel_time);
		case 0xC8: // CMP X,imm
			nz = x - data;
			c = ~nz;
			nz &= 0xFF;
			goto inc_pc_loop;

		case 0x7E: // CMP Y,dp
			data += dp;
			goto cmp_y_addr;
		case 0x5E: // CMP Y,abs
			data = get_le16(pc);
			++pc;
		cmp_y_addr:
			data = this->cpu_read(data, rel_time);
		case 0xAD: // CMP Y,imm
			nz = y - data;
			c = ~nz;
			nz &= 0xFF;
			goto inc_pc_loop;

		{
			int addr;
			case 0xB9: // SBC (x),(y)
			case 0x99: // ADC (x),(y)
				--pc; // compensate for inc later
				data = this->cpu_read(DP_ADDR(y), rel_time - 2);
				addr = x + dp;
				goto adc_addr;
			case 0xA9: // SBC dp,dp
			case 0x89: // ADC dp,dp
				data = this->cpu_read(DP_ADDR(data), rel_time - 3);
			case 0xB8: // SBC dp,imm
			case 0x98: // ADC dp,imm
				addr = *(++pc) + dp;
			adc_addr:
				nz = this->cpu_read(addr, rel_time - 1);
				goto adc_data;

			// catch ADC and SBC together, then decode later based on operand
#undef CASE
#define CASE(n) case n: case (n) + 0x20:
				ADDR_MODES(0x88) // ADC/SBC addr
				data = this->cpu_read(data, rel_time);
			case 0xA8: // SBC imm
			case 0x88: // ADC imm
				addr = -1; // A
				nz = a;
			adc_data:
			{
				int flags;
				if (opcode >= 0xA0) // SBC
					data ^= 0xFF;

				flags = data ^ nz;
				nz += data + (c >> 8 & 1);
				flags ^= nz;

				psw = (psw & ~(v40 | h08)) | ((flags >> 1) & h08) | (((flags + 0x80) >> 2) & v40);
				c = nz;
				if (addr < 0)
				{
					a = static_cast<uint8_t>(nz);
					goto inc_pc_loop;
				}
				this->cpu_write(/*(uint8_t)*/nz, addr, rel_time);
				goto inc_pc_loop;
			}
		}

		// 6. ADDITION & SUBTRACTION COMMANDS

#define INC_DEC_REG(reg, op) \
	nz = reg op; \
	reg = static_cast<uint8_t>(nz); \
	goto loop;

		case 0xBC: INC_DEC_REG(a, + 1) // INC A
		case 0x3D: INC_DEC_REG(x, + 1) // INC X
		case 0xFC: INC_DEC_REG(y, + 1) // INC Y

		case 0x9C: INC_DEC_REG(a, - 1) // DEC A
		case 0x1D: INC_DEC_REG(x, - 1) // DEC X
		case 0xDC: INC_DEC_REG(y, - 1) // DEC Y

		case 0x9B: // DEC dp+X
		case 0xBB: // INC dp+X
			data = static_cast<uint8_t>(data + x);
		case 0x8B: // DEC dp
		case 0xAB: // INC dp
			data += dp;
			goto inc_abs;
		case 0x8C: // DEC abs
		case 0xAC: // INC abs
			data = get_le16(pc);
			++pc;
		inc_abs:
			nz = ((opcode >> 4) & 2) - 1;
			nz += this->cpu_read(data, rel_time - 1);
			this->cpu_write(/*(uint8_t)*/ nz, data, rel_time);
			goto inc_pc_loop;

		// 7. SHIFT, ROTATION COMMANDS

		case 0x5C: // LSR A
			c = 0;
		case 0x7C: // ROR A
		{
			nz = ((c >> 1) & 0x80) | (a >> 1);
			c = a << 8;
			a = nz;
			goto loop;
		}

		case 0x1C: // ASL A
			c = 0;
		case 0x3C: // ROL A
		{
			int temp = c >> 8 & 1;
			c = a << 1;
			nz = c | temp;
			a = static_cast<uint8_t>(nz);
			goto loop;
		}

		case 0x0B: // ASL dp
			c = 0;
			data += dp;
			goto rol_mem;
		case 0x1B: // ASL dp+X
			c = 0;
		case 0x3B: // ROL dp+X
			data = static_cast<uint8_t>(data + x);
		case 0x2B: // ROL dp
			data += dp;
			goto rol_mem;
		case 0x0C: // ASL abs
			c = 0;
		case 0x2C: // ROL abs
			data = get_le16(pc);
			++pc;
		rol_mem:
			nz = c >> 8 & 1;
			nz |= (c = this->cpu_read(data, rel_time - 1) << 1);
			this->cpu_write(/*(uint8_t)*/ nz, data, rel_time);
			goto inc_pc_loop;

		case 0x4B: // LSR dp
			c = 0;
			data += dp;
			goto ror_mem;
		case 0x5B: // LSR dp+X
			c = 0;
		case 0x7B: // ROR dp+X
			data = static_cast<uint8_t>(data + x);
		case 0x6B: // ROR dp
			data += dp;
			goto ror_mem;
		case 0x4C: // LSR abs
			c = 0;
		case 0x6C: // ROR abs
			data = get_le16(pc);
			++pc;
		ror_mem:
		{
			int temp = this->cpu_read(data, rel_time - 1);
			nz = (c >> 1 & 0x80) | (temp >> 1);
			c = temp << 8;
			this->cpu_write(nz, data, rel_time);
			goto inc_pc_loop;
		}

		case 0x9F: // XCN
			nz = a = (a >> 4) | static_cast<uint8_t>(a << 4);
			goto loop;

		// 8. 16-BIT TRANSMISION COMMANDS

		case 0xBA: // MOVW YA,dp
			a = this->cpu_read(DP_ADDR(data), rel_time - 2);
			nz = (a & 0x7F) | (a >> 1);
			y = this->cpu_read(DP_ADDR(static_cast<uint8_t>(data + 1)), rel_time);
			nz |= y;
			goto inc_pc_loop;

		case 0xDA: // MOVW dp,YA
			this->cpu_write(a, DP_ADDR(data), rel_time - 1);
			this->cpu_write(y + no_read_before_write, DP_ADDR(static_cast<uint8_t>(data + 1)), rel_time);
			goto inc_pc_loop;

		// 9. 16-BIT OPERATION COMMANDS

		case 0x3A: // INCW dp
		case 0x1A: // DECW dp
		{
			int temp;
			// low byte
			data += dp;
			temp = this->cpu_read(data, rel_time - 3);
			temp += (opcode >> 4 & 2) - 1; // +1 for INCW, -1 for DECW
			nz = ((temp >> 1) | temp) & 0x7F;
			this->cpu_write(/*(uint8_t)*/ temp, data, rel_time - 2);

			// high byte
			data = static_cast<uint8_t>(data + 1) + dp;
			temp = static_cast<uint8_t>((temp >> 8) + this->cpu_read(data, rel_time - 1));
			nz |= temp;
			this->cpu_write(temp, data, rel_time);

			goto inc_pc_loop;
		}

		case 0x7A: // ADDW YA,dp
		case 0x9A: // SUBW YA,dp
		{
			int lo = this->cpu_read(DP_ADDR(data), rel_time - 2);
			int hi = this->cpu_read(DP_ADDR(static_cast<uint8_t>(data + 1)), rel_time);

			if (opcode == 0x9A) // SUBW
			{
				lo = (lo ^ 0xFF) + 1;
				hi ^= 0xFF;
			}

			lo += a;
			int result = y + hi + (lo >> 8);
			int flags = hi ^ y ^ result;

			psw = (psw & ~(v40 | h08)) | (flags >> 1 & h08) | ((flags + 0x80) >> 2 & v40);
			c = result;
			a = static_cast<uint8_t>(lo);
			result = static_cast<uint8_t>(result);
			y = result;
			nz = (((lo >> 1) | lo) & 0x7F) | result;

			goto inc_pc_loop;
		}

		case 0x5A: // CMPW YA,dp
		{
			int temp = a - this->cpu_read(DP_ADDR(data), rel_time - 1);
			nz = ((temp >> 1) | temp) & 0x7F;
			temp = y + (temp >> 8);
			temp -= this->cpu_read(DP_ADDR(static_cast<uint8_t>(data + 1)), rel_time);
			nz |= temp;
			c  = ~temp;
			nz &= 0xFF;
			goto inc_pc_loop;
		}

		// 10. MULTIPLICATION & DIVISON COMMANDS

		case 0xCF: // MUL YA
		{
			unsigned temp = y * a;
			a = static_cast<uint8_t>(temp);
			nz = ((temp >> 1) | temp) & 0x7F;
			y = temp >> 8;
			nz |= y;
			goto loop;
		}

		case 0x9E: // DIV YA,X
		{
			unsigned ya = y * 0x100 + a;

			psw &= ~(h08 | v40);

			if (y >= x)
				psw |= v40;

			if ((y & 15) >= (x & 15))
				psw |= h08;

			if (y < x * 2)
			{
				a = ya / x;
				y = ya - a * x;
			}
			else
			{
				a = 255 - (ya - x * 0x200) / (256 - x);
				y = x + (ya - x * 0x200) % (256 - x);
			}

			nz = static_cast<uint8_t>(a);
			a = static_cast<uint8_t>(a);

			goto loop;
		}

		// 11. DECIMAL COMPENSATION COMMANDS

		case 0xDF: // DAA
			if (a > 0x99 || c & 0x100)
			{
				a += 0x60;
				c = 0x100;
			}

			if ((a & 0x0F) > 9 || psw & h08)
				a += 0x06;

			nz = a;
			a = static_cast<uint8_t>(a);
			goto loop;

		case 0xBE: // DAS
			if (a > 0x99 || !(c & 0x100))
			{
				a -= 0x60;
				c = 0;
			}

			if ((a & 0x0F) > 9 || !(psw & h08))
				a -= 0x06;

			nz = a;
			a = static_cast<uint8_t>(a);
			goto loop;

		// 12. BRANCHING COMMANDS

		case 0x2F: // BRA rel
			pc += static_cast<int8_t>(data);
			goto inc_pc_loop;

		case 0x30: // BMI
			BRANCH(nz & nz_neg_mask)

		case 0x10: // BPL
			BRANCH(!(nz & nz_neg_mask))

		case 0xB0: // BCS
			BRANCH(c & 0x100)

		case 0x90: // BCC
			BRANCH(!(c & 0x100))

		case 0x70: // BVS
			BRANCH(psw & v40)

		case 0x50: // BVC
			BRANCH(!(psw & v40))

#define CBRANCH(cond) \
{ \
	++pc; \
	if (cond) \
		goto cbranch_taken_loop; \
	rel_time -= 2; \
	goto inc_pc_loop; \
}

		case 0x03: // BBS dp.bit,rel
		case 0x23:
		case 0x43:
		case 0x63:
		case 0x83:
		case 0xA3:
		case 0xC3:
		case 0xE3:
			CBRANCH((this->cpu_read(DP_ADDR(data), rel_time - 4) >> (opcode >> 5)) & 1)

		case 0x13: // BBC dp.bit,rel
		case 0x33:
		case 0x53:
		case 0x73:
		case 0x93:
		case 0xB3:
		case 0xD3:
		case 0xF3:
			CBRANCH(!((this->cpu_read(DP_ADDR(data), rel_time - 4) >> (opcode >> 5)) & 1))

		case 0xDE: // CBNE dp+X,rel
			data = static_cast<uint8_t>(data + x);
			// fall through
		case 0x2E: // CBNE dp,rel
		{
			// 61% from timer
			int temp = CPU_READ_TIMER(-4, DP_ADDR(data));
			CBRANCH(temp != a)
		}

		case 0x6E: // DBNZ dp,rel
		{
			unsigned temp = this->cpu_read(DP_ADDR(data), rel_time - 4) - 1;
			this->cpu_write(/*(uint8_t)*/ temp + no_read_before_write, DP_ADDR(static_cast<uint8_t>(data)), rel_time - 3);
			CBRANCH(temp)
		}

		case 0xFE: // DBNZ Y,rel
			y = static_cast<uint8_t>(y - 1);
			BRANCH(y)

		case 0x1F: // JMP [abs+X]
			SET_PC(get_le16(pc) + x);
			// fall through
		case 0x5F: // JMP abs
			SET_PC(get_le16(pc));
			goto loop;

		// 13. SUB-ROUTINE CALL RETURN COMMANDS

		case 0x0F: // BRK
		{
			int temp;
			int ret_addr = GET_PC();
			SET_PC(READ_PROG16(0xFFDE)); // vector address verified
			PUSH16(ret_addr);
			GET_PSW(temp);
			psw = (psw | b10) & ~i04;
			PUSH(temp);
			goto loop;
		}

		case 0x4F: // PCALL offset
		{
			int ret_addr = GET_PC() + 1;
			SET_PC(0xFF00 | data);
			PUSH16(ret_addr);
			goto loop;
		}

		case 0x01: // TCALL n
		case 0x11:
		case 0x21:
		case 0x31:
		case 0x41:
		case 0x51:
		case 0x61:
		case 0x71:
		case 0x81:
		case 0x91:
		case 0xA1:
		case 0xB1:
		case 0xC1:
		case 0xD1:
		case 0xE1:
		case 0xF1:
		{
			int ret_addr = GET_PC();
			SET_PC(READ_PROG16(0xFFDE - (opcode >> 3)));
			PUSH16(ret_addr);
			goto loop;
		}

		// 14. STACK OPERATION COMMANDS

		{
			int temp;
		case 0x7F: // RET1
			temp = *sp;
			SET_PC(get_le16(sp + 1));
			sp += 3;
			goto set_psw;
		case 0x8E: // POP PSW
			POP(temp);
		set_psw:
			SET_PSW(temp);
			goto loop;
		}

		case 0x0D: // PUSH PSW
		{
			int temp;
			GET_PSW(temp);
			PUSH(temp);
			goto loop;
		}

		case 0x2D: // PUSH A
			PUSH(a);
			goto loop;

		case 0x4D: // PUSH X
			PUSH(x);
			goto loop;

		case 0x6D: // PUSH Y
			PUSH(y);
			goto loop;

		case 0xAE: // POP A
			POP(a);
			goto loop;

		case 0xCE: // POP X
			POP(x);
			goto loop;

		case 0xEE: // POP Y
			POP(y);
			goto loop;

		// 15. BIT OPERATION COMMANDS

		case 0x02: // SET1
		case 0x22:
		case 0x42:
		case 0x62:
		case 0x82:
		case 0xA2:
		case 0xC2:
		case 0xE2:
		case 0x12: // CLR1
		case 0x32:
		case 0x52:
		case 0x72:
		case 0x92:
		case 0xB2:
		case 0xD2:
		case 0xF2:
		{
			int bit = 1 << (opcode >> 5);
			int mask = ~bit;
			if (opcode & 0x10)
				bit = 0;
			data += dp;
			this->cpu_write((this->cpu_read(data, rel_time - 1) & mask) | bit, data, rel_time);
			goto inc_pc_loop;
		}

		case 0x0E: // TSET1 abs
		case 0x4E: // TCLR1 abs
			data = get_le16(pc);
			pc += 2;
			{
				unsigned temp = this->cpu_read(data, rel_time - 2);
				nz = static_cast<uint8_t>(a - temp);
				temp &= ~a;
				if (opcode == 0x0E)
					temp |= a;
				this->cpu_write(temp, data, rel_time);
			}
			goto loop;

		case 0x4A: // AND1 C,mem.bit
			c &= MEM_BIT(0);
			pc += 2;
			goto loop;

		case 0x6A: // AND1 C,/mem.bit
			c &= ~MEM_BIT(0);
			pc += 2;
			goto loop;

		case 0x0A: // OR1 C,mem.bit
			c |= MEM_BIT(-1);
			pc += 2;
			goto loop;

		case 0x2A: // OR1 C,/mem.bit
			c |= ~MEM_BIT(-1);
			pc += 2;
			goto loop;

		case 0x8A: // EOR1 C,mem.bit
			c ^= MEM_BIT(-1);
			pc += 2;
			goto loop;

		case 0xEA: // NOT1 mem.bit
			data = get_le16(pc);
			pc += 2;
			{
				unsigned temp = this->cpu_read(data & 0x1FFF, rel_time - 1);
				temp ^= 1 << (data >> 13);
				this->cpu_write(temp, data & 0x1FFF, rel_time);
			}
			goto loop;

		case 0xCA: // MOV1 mem.bit,C
			data = get_le16(pc);
			pc += 2;
			{
				unsigned temp = this->cpu_read(data & 0x1FFF, rel_time - 2);
				unsigned bit = data >> 13;
				temp = (temp & ~(1 << bit)) | ((c >> 8 & 1) << bit);
				this->cpu_write(temp + no_read_before_write, data & 0x1FFF, rel_time);
			}
			goto loop;

		case 0xAA: // MOV1 C,mem.bit
			c = MEM_BIT(0);
			pc += 2;
			goto loop;

		// 16. PROGRAM PSW FLAG OPERATION COMMANDS

		case 0x60: // CLRC
			c = 0;
			goto loop;

		case 0x80: // SETC
			c = ~0;
			goto loop;

		case 0xED: // NOTC
			c ^= 0x100;
			goto loop;

		case 0xE0: // CLRV
			psw &= ~(v40 | h08);
			goto loop;

		case 0x20: // CLRP
			dp = 0;
			goto loop;

		case 0x40: // SETP
			dp = 0x100;
			goto loop;

		case 0xA0: // EI
			psw |= i04;
			goto loop;

		case 0xC0: // DI
			psw &= ~i04;
			goto loop;

		// 17. OTHER COMMANDS

		case 0x00: // NOP
			goto loop;

		case 0xFF: // STOP
		{
			// handle PC wrap-around
			unsigned addr = GET_PC() - 1;
			if (addr >= 0x10000)
			{
				addr &= 0xFFFF;
				SET_PC(addr);
				goto loop;
			}
		}
		// fall through
		case 0xEF: // SLEEP
			--pc;
			rel_time = 0;
			goto stop;
	} // switch

	assert(0); // catch any unhandled instructions

out_of_time:
	rel_time -= this->m.cycle_table[*pc]; // undo partial execution of opcode
stop:

	// Uncache registers
	m.cpu_regs.pc = static_cast<uint16_t>(GET_PC());
	m.cpu_regs.sp = static_cast<uint8_t>(GET_SP());
	m.cpu_regs.a = static_cast<uint8_t>(a);
	m.cpu_regs.x = static_cast<uint8_t>(x);
	m.cpu_regs.y = static_cast<uint8_t>(y);
	int temp;
	GET_PSW(temp);
	m.cpu_regs.psw = static_cast<uint8_t>(temp);

	this->m.spc_time += rel_time;
	this->m.dsp_time -= rel_time;
	this->m.timers[0].next_time -= rel_time;
	this->m.timers[1].next_time -= rel_time;
	this->m.timers[2].next_time -= rel_time;
	/*assert( m.spc_time >= end_time );*/
	return &this->m.smp_regs[0][r_cpuio0];
}
