#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

namespace SNES
{
	uint8_t SMP::op_adc(uint8_t x, uint8_t y)
	{
		int r = x + y + this->regs.p.c;
		this->regs.p.n = r & 0x80;
		this->regs.p.v = ~(x ^ y) & (x ^ r) & 0x80;
		this->regs.p.h = (x ^ y ^ r) & 0x10;
		this->regs.p.z = static_cast<uint8_t>(r) == 0;
		this->regs.p.c = r > 0xff;
		return r;
	}

	uint16_t SMP::op_addw(uint16_t x, uint16_t y)
	{
		this->regs.p.c = 0;
		uint16_t r = this->op_adc(x, y);
		r |= this->op_adc(x >> 8, y >> 8) << 8;
		regs.p.z = r == 0;
		return r;
	}

	uint8_t SMP::op_and(uint8_t x, uint8_t y)
	{
		x &= y;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_cmp(uint8_t x, uint8_t y)
	{
		int r = x - y;
		regs.p.n = r & 0x80;
		regs.p.z = static_cast<uint8_t>(r) == 0;
		regs.p.c = r >= 0;
		return x;
	}

	uint16_t SMP::op_cmpw(uint16_t x, uint16_t y)
	{
		int r = x - y;
		regs.p.n = r & 0x8000;
		regs.p.z = static_cast<uint16_t>(r) == 0;
		regs.p.c = r >= 0;
		return x;
	}

	uint8_t SMP::op_eor(uint8_t x, uint8_t y)
	{
		x ^= y;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_or(uint8_t x, uint8_t y)
	{
		x |= y;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_sbc(uint8_t x, uint8_t y)
	{
		int r = x - y - !regs.p.c;
		regs.p.n = r & 0x80;
		regs.p.v = (x ^ y) & (x ^ r) & 0x80;
		regs.p.h = !((x ^ y ^ r) & 0x10);
		regs.p.z = static_cast<uint8_t>(r) == 0;
		regs.p.c = r >= 0;
		return r;
	}

	uint16_t SMP::op_subw(uint16_t x, uint16_t y)
	{
		regs.p.c = 1;
		uint16_t r = this->op_sbc(x, y);
		r |= this->op_sbc(x >> 8, y >> 8) << 8;
		regs.p.z = r == 0;
		return r;
	}

	uint8_t SMP::op_inc(uint8_t x)
	{
		++x;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_dec(uint8_t x)
	{
		--x;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_asl(uint8_t x)
	{
		regs.p.c = x & 0x80;
		x <<= 1;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_lsr(uint8_t x)
	{
		regs.p.c = x & 0x01;
		x >>= 1;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_rol(uint8_t x)
	{
		unsigned carry = static_cast<unsigned>(regs.p.c);
		regs.p.c = x & 0x80;
		x = (x << 1) | carry;
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}

	uint8_t SMP::op_ror(uint8_t x)
	{
		unsigned carry = static_cast<unsigned>(regs.p.c) << 7;
		regs.p.c = x & 0x01;
		x = carry | (x >> 1);
		regs.p.n = x & 0x80;
		regs.p.z = x == 0;
		return x;
	}
}
