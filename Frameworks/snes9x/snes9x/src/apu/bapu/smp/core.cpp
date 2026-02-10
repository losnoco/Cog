#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

namespace SNES
{
	void SMP::tick()
	{
		this->timer0.tick();
		this->timer1.tick();
		this->timer2.tick();

		++this->clock;
		++this->SPC->dsp->clock;
	}

	void SMP::tick(unsigned clocks)
	{
		this->timer0.tick(clocks);
		this->timer1.tick(clocks);
		this->timer2.tick(clocks);

		this->clock += clocks;
		this->SPC->dsp->clock += clocks;
	}

	void SMP::op_io()
	{
		this->tick();
	}

	void SMP::op_io(unsigned clocks)
	{
		this->tick(clocks);
	}

	uint8_t SMP::op_read(uint16_t addr)
	{
		this->tick();
		if ((addr & 0xfff0) == 0x00f0)
			return this->mmio_read(addr);
		if (addr >= 0xffc0 && this->status.iplrom_enable)
			return SMP::iplrom[addr & 0x3f];
		return this->apuram[addr];
	}

	void SMP::op_write(uint16_t addr, uint8_t data)
	{
		this->tick();
		if ((addr & 0xfff0) == 0x00f0)
			this->mmio_write(addr, data);
		this->apuram[addr] = data; // all writes go to RAM, even MMIO writes
	}

	uint8_t SMP::op_readstack()
	{
		this->tick();
		return this->apuram[0x0100 | ++this->regs.sp];
	}

	void SMP::op_writestack(uint8_t data)
	{
		this->tick();
		this->apuram[0x0100 | this->regs.sp--] = data;
	}

	void SMP::op_step()
	{
		const auto op_readpc = [&]() { return this->op_read(this->regs.pc++); };
		const auto op_readdp = [&](uint16_t addr) { return this->op_read((this->regs.p.p << 8) + (addr & 0xff)); };
		const auto op_writedp = [&](uint16_t addr, uint8_t data) { this->op_write((this->regs.p.p << 8) + (addr & 0xff), data); };
		const auto op_readaddr = [&](uint16_t addr) { return this->op_read(addr); };
		const auto op_writeaddr = [&](uint16_t addr, uint8_t data) { this->op_write(addr, data); };

		if (!this->opcode_cycle)
			this->opcode_number = op_readpc();

		switch (opcode_number)
		{
			// Start of core/oppseudo_misc.cpp

			case 0x00:
				this->op_io();
				break;

			case 0xef:
			case 0xff:
				this->op_io(2);
				--this->regs.pc;
				break;

			case 0x9f:
				this->op_io(4);
				this->regs.B.a = (this->regs.B.a >> 4) | (this->regs.B.a << 4);
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			case 0xdf:
				this->op_io(2);
				if (this->regs.p.c || this->regs.B.a > 0x99)
				{
					this->regs.B.a += 0x60;
					this->regs.p.c = 1;
				}
				if (this->regs.p.h || (this->regs.B.a & 15) > 0x09)
					this->regs.B.a += 0x06;
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			case 0xbe:
				this->op_io(2);
				if (!this->regs.p.c || this->regs.B.a > 0x99)
				{
					this->regs.B.a -= 0x60;
					this->regs.p.c = 0;
				}
				if (!this->regs.p.h || (this->regs.B.a & 15) > 0x09)
					this->regs.B.a -= 0x06;
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			case 0x60:
			case 0x80:
				this->op_io();
				this->regs.p.c = opcode_number == 0x80;
				break;

			case 0x20:
			case 0x40:
				this->op_io();
				this->regs.p.p = opcode_number == 0x40;
				break;

			case 0xe0:
				this->op_io();
				this->regs.p.v = 0;
				this->regs.p.h = 0;
				break;

			case 0xed:
				this->op_io(2);
				this->regs.p.c = !this->regs.p.c;
				break;

			case 0xa0:
			case 0xc0:
				this->op_io(2);
				this->regs.p.i = opcode_number == 0xa0;
				break;

			case 0x02:
			case 0x22:
			case 0x42:
			case 0x62:
			case 0x82:
			case 0xa2:
			case 0xc2:
			case 0xe2:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd |= 0x01 << ((opcode_number & 0xE0) >> 5);
				op_writedp(this->dp, this->rd);
				break;

			case 0x12:
			case 0x32:
			case 0x52:
			case 0x72:
			case 0x92:
			case 0xb2:
			case 0xd2:
			case 0xf2:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd &= ~(0x01 << ((opcode_number & 0xE0) >> 5));
				op_writedp(this->dp, this->rd);
				break;

			case 0x2d:
				this->op_io(2);
				this->op_writestack(this->regs.B.a);
				break;

			case 0x4d:
				this->op_io(2);
				this->op_writestack(this->regs.x);
				break;

			case 0x6d:
				this->op_io(2);
				this->op_writestack(this->regs.B.y);
				break;

			case 0x0d:
				this->op_io(2);
				this->op_writestack(this->regs.p);
				break;

			case 0xae:
				this->op_io(2);
				this->regs.B.a = this->op_readstack();
				break;

			case 0xce:
				this->op_io(2);
				this->regs.x = this->op_readstack();
				break;

			case 0xee:
				this->op_io(2);
				this->regs.B.y = this->op_readstack();
				break;

			case 0x8e:
				this->op_io(2);
				this->regs.p = this->op_readstack();
				break;

			case 0xcf:
				this->op_io(8);
				this->ya = this->regs.B.y * this->regs.B.a;
				this->regs.B.a = this->ya;
				this->regs.B.y = this->ya >> 8;
				// result is set based on y (high-byte) only
				this->regs.p.n = !!(this->regs.B.y & 0x80);
				this->regs.p.z = !this->regs.B.y;
				break;

			case 0x9e:
				this->op_io(11);
				this->ya = this->regs.ya;
				// overflow set if quotient >= 256
				this->regs.p.v = !!(this->regs.B.y >= this->regs.x);
				this->regs.p.h = !!((this->regs.B.y & 15) >= (this->regs.x & 15));
				if (this->regs.B.y < (this->regs.x << 1))
				{
					// if quotient is <= 511 (will fit into 9-bit result)
					this->regs.B.a = this->ya / this->regs.x;
					this->regs.B.y = this->ya % this->regs.x;
				}
				else
				{
					// otherwise, the quotient won't fit into regs.p.v + regs.B.a
					// this emulates the odd behavior of the S-SMP in this case
					this->regs.B.a = 255 - (this->ya - (this->regs.x << 9)) / (256 - this->regs.x);
					this->regs.B.y = this->regs.x + (this->ya - (this->regs.x << 9)) % (256 - this->regs.x);
				}
				// result is set based on a (quotient) only
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			// End of core/oppseudo_misc.cpp

			// Start of core/oppseudo_mov.cpp

			case 0x7d:
				this->op_io();
				this->regs.B.a = this->regs.x;
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			case 0xdd:
				this->op_io();
				this->regs.B.a = this->regs.B.y;
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			case 0x5d:
				this->op_io();
				this->regs.x = this->regs.B.a;
				this->regs.p.n = !!(this->regs.x & 0x80);
				this->regs.p.z = !this->regs.x;
				break;

			case 0xfd:
				this->op_io();
				this->regs.B.y = this->regs.B.a;
				this->regs.p.n = !!(this->regs.B.y & 0x80);
				this->regs.p.z = !this->regs.B.y;
				break;

			case 0x9d:
				this->op_io();
				this->regs.x = this->regs.sp;
				this->regs.p.n = !!(this->regs.x & 0x80);
				this->regs.p.z = !this->regs.x;
				break;

			case 0xbd:
				this->op_io();
				this->regs.sp = this->regs.x;
				break;

			case 0xe8:
				this->regs.B.a = op_readpc();
				this->regs.p.n = !!(this->regs.B.a & 0x80);
				this->regs.p.z = !this->regs.B.a;
				break;

			case 0xcd:
				this->regs.x = op_readpc();
				this->regs.p.n = !!(this->regs.x & 0x80);
				this->regs.p.z = !this->regs.x;
				break;

			case 0x8d:
				this->regs.B.y = op_readpc();
				this->regs.p.n = !!(this->regs.B.y & 0x80);
				this->regs.p.z = !this->regs.B.y;
				break;

			case 0xe6:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->op_io();
						break;
					case 2:
						this->regs.B.a = op_readdp(this->regs.x);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xbf:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->op_io();
						break;
					case 2:
						this->regs.B.a = op_readdp(this->regs.x++);
						this->op_io();
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xe4:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->regs.B.a = op_readdp(this->sp);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xf8:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->regs.x = op_readdp(this->sp);
						this->regs.p.n = !!(this->regs.x & 0x80);
						this->regs.p.z = !this->regs.x;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xeb:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->regs.B.y = op_readdp(this->sp);
						this->regs.p.n = !!(this->regs.B.y & 0x80);
						this->regs.p.z = !this->regs.B.y;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xf4:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->op_io();
						break;
					case 2:
						this->regs.B.a = op_readdp(this->sp + this->regs.x);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xf9:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->op_io();
						break;
					case 2:
						this->regs.x = op_readdp(this->sp + this->regs.B.y);
						this->regs.p.n = !!(this->regs.x & 0x80);
						this->regs.p.z = !this->regs.x;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xfb:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->op_io();
						break;
					case 2:
						this->regs.B.y = op_readdp(this->sp + this->regs.x);
						this->regs.p.n = !!(this->regs.B.y & 0x80);
						this->regs.p.z = !this->regs.B.y;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xe5:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->sp |= op_readpc() << 8;
						break;
					case 3:
						this->regs.B.a = op_readaddr(this->sp);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xe9:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->sp |= op_readpc() << 8;
						break;
					case 2:
						this->regs.x = op_readaddr(this->sp);
						this->regs.p.n = !!(this->regs.x & 0x80);
						this->regs.p.z = !this->regs.x;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xec:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->sp |= op_readpc() << 8;
						break;
					case 2:
						this->regs.B.y = op_readaddr(this->sp);
						this->regs.p.n = !!(this->regs.B.y & 0x80);
						this->regs.p.z = !this->regs.B.y;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xf5:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->sp |= op_readpc() << 8;
						this->op_io();
						break;
					case 2:
						this->regs.B.a = op_readaddr(this->sp + this->regs.x);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xf6:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->sp |= op_readpc() << 8;
						this->op_io();
						break;
					case 2:
						this->regs.B.a = op_readaddr(this->sp + this->regs.B.y);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xe7:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc() + this->regs.x;
						this->op_io();
						break;
					case 2:
						this->sp = op_readdp(this->dp);
						break;
					case 3:
						this->sp |= op_readdp(this->dp + 1) << 8;
						break;
					case 4:
						this->regs.B.a = op_readaddr(this->sp);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xf7:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->op_io();
						break;
					case 2:
						this->sp = op_readdp(this->dp);
						break;
					case 3:
						this->sp |= op_readdp(this->dp + 1) << 8;
						break;
					case 4:
						this->regs.B.a = op_readaddr(this->sp + this->regs.B.y);
						this->regs.p.n = !!(this->regs.B.a & 0x80);
						this->regs.p.z = !this->regs.B.a;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xfa:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->rd = op_readdp(this->sp);
						break;
					case 3:
						this->dp = op_readpc();
						break;
					case 4:
						op_writedp(this->dp, this->rd);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0x8f:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->rd = op_readpc();
						this->dp = op_readpc();
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->rd);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xc6:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->op_io();
						break;
					case 2:
						op_readdp(this->regs.x);
						break;
					case 3:
						op_writedp(this->regs.x, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xaf:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->op_io(2);
						break;
					case 2:
						op_writedp(this->regs.x++, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xc4:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xd8:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.x);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xcb:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.B.y);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xd4:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->op_io();
						this->dp += this->regs.x;
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xd9:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->op_io();
						this->dp += this->regs.B.y;
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.x);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xdb:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->op_io();
						this->dp += this->regs.x;
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.B.y);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xc5:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						this->dp |= op_readpc() << 8;
						break;
					case 3:
						op_readaddr(this->dp);
						break;
					case 4:
						op_writeaddr(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xc9:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						this->dp |= op_readpc() << 8;
						break;
					case 3:
						op_readaddr(this->dp);
						break;
					case 4:
						op_writeaddr(this->dp, this->regs.x);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xcc:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						this->dp |= op_readpc() << 8;
						break;
					case 3:
						op_readaddr(this->dp);
						break;
					case 4:
						op_writeaddr(this->dp, this->regs.B.y);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xd5:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->dp |= op_readpc() << 8;
						this->op_io();
						this->dp += this->regs.x;
						break;
					case 2:
						op_readaddr(this->dp);
						break;
					case 3:
						op_writeaddr(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xd6:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->dp |= op_readpc() << 8;
						this->op_io();
						this->dp += this->regs.B.y;
						break;
					case 2:
						op_readaddr(this->dp);
						break;
					case 3:
						op_writeaddr(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xc7:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->op_io();
						this->sp += this->regs.x;
						break;
					case 2:
						this->dp = op_readdp(this->sp);
						break;
					case 3:
						this->dp |= op_readdp(this->sp + 1) << 8;
						break;
					case 4:
						op_readaddr(this->dp);
						break;
					case 5:
						op_writeaddr(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xd7:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->dp = op_readdp(this->sp);
						break;
					case 3:
						this->dp |= op_readdp(this->sp + 1) << 8;
						this->op_io();
						this->dp += this->regs.B.y;
						break;
					case 4:
						op_readaddr(this->dp);
						break;
					case 5:
						op_writeaddr(this->dp, this->regs.B.a);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xba:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						break;
					case 2:
						this->regs.B.a = op_readdp(this->sp);
						this->op_io();
						break;
					case 3:
						this->regs.B.y = op_readdp(this->sp + 1);
						this->regs.p.n = !!(this->regs.ya & 0x8000);
						this->regs.p.z = !this->regs.ya;
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xda:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						op_readdp(this->dp);
						break;
					case 3:
						op_writedp(this->dp, this->regs.B.a);
						break;
					case 4:
						op_writedp(this->dp + 1, this->regs.B.y);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xaa:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->sp = op_readpc();
						this->sp |= op_readpc() << 8;
						break;
					case 2:
						this->bit = this->sp >> 13;
						this->sp &= 0x1fff;
						this->rd = op_readaddr(this->sp);
						this->regs.p.c = !!(this->rd & (1 << this->bit));
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0xca:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						this->dp |= op_readpc() << 8;
						break;
					case 2:
						this->bit = this->dp >> 13;
						this->dp &= 0x1fff;
						this->rd = op_readaddr(this->dp);
						if (this->regs.p.c)
							this->rd |= (1 << this->bit);
						else
							this->rd &= ~(1 << this->bit);
						this->op_io();
						break;
					case 3:
						op_writeaddr(this->dp, this->rd);
						this->opcode_cycle = 0;
						break;
				}
				break;

			// End of core/oppseudo_mov.cpp

			// Start of core/oppseudo_pc.cpp

			case 0x2f:
				this->rd = op_readpc();
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0xf0:
				this->rd = op_readpc();
				if (!this->regs.p.z)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0xd0:
				this->rd = op_readpc();
				if (this->regs.p.z)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0xb0:
				this->rd = op_readpc();
				if (!this->regs.p.c)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x90:
				this->rd = op_readpc();
				if (this->regs.p.c)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x70:
				this->rd = op_readpc();
				if (!this->regs.p.v)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x50:
				this->rd = op_readpc();
				if (this->regs.p.v)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x30:
				this->rd = op_readpc();
				if (!this->regs.p.n)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x10:
				this->rd = op_readpc();
				if (this->regs.p.n)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(rd);
				break;

			case 0x03:
			case 0x23:
			case 0x43:
			case 0x63:
			case 0x83:
			case 0xa3:
			case 0xc3:
			case 0xe3:
			{
				this->dp = op_readpc();
				this->sp = op_readdp(this->dp);
				this->rd = op_readpc();
				this->op_io();
				uint8_t mask = 1 << ((opcode_number & 0xE0) >> 5);
				if ((sp & mask) != mask)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;
			}

			case 0x13:
			case 0x33:
			case 0x53:
			case 0x73:
			case 0x93:
			case 0xb3:
			case 0xd3:
			case 0xf3:
			{
				this->dp = op_readpc();
				this->sp = op_readdp(this->dp);
				this->rd = op_readpc();
				this->op_io();
				uint8_t mask = 1 << ((opcode_number & 0xE0) >> 5);
				if ((sp & mask) == mask)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;
			}

			case 0x2e:
				this->dp = op_readpc();
				this->sp = op_readdp(this->dp);
				this->rd = op_readpc();
				this->op_io();
				if (this->regs.B.a == this->sp)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0xde:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp + this->regs.x);
				this->rd = op_readpc();
				this->op_io();
				if (this->regs.B.a == this->sp)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x6e:
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				op_writedp(this->dp, --this->wr);
				this->rd = op_readpc();
				if (!this->wr)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0xfe:
				this->rd = op_readpc();
				this->op_io();
				--this->regs.B.y;
				this->op_io();
				if (!this->regs.B.y)
					break;
				this->op_io(2);
				this->regs.pc += static_cast<int8_t>(this->rd);
				break;

			case 0x5f:
				this->rd = op_readpc();
				this->rd |= op_readpc() << 8;
				this->regs.pc = this->rd;
				break;

			case 0x1f:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->dp += this->regs.x;
				this->rd = op_readaddr(this->dp);
				this->rd |= op_readaddr(this->dp + 1) << 8;
				this->regs.pc = this->rd;
				break;

			case 0x3f:
				this->rd = op_readpc();
				this->rd |= op_readpc() << 8;
				this->op_io(3);
				this->op_writestack(this->regs.pc >> 8);
				this->op_writestack(this->regs.pc);
				this->regs.pc = this->rd;
				break;

			case 0x4f:
				this->rd = op_readpc();
				this->op_io(2);
				this->op_writestack(this->regs.pc >> 8);
				this->op_writestack(this->regs.pc);
				this->regs.pc = 0xff00 | this->rd;
				break;

			case 0x01:
			case 0x11:
			case 0x21:
			case 0x31:
			case 0x41:
			case 0x51:
			case 0x61:
			case 0x71:
			case 0x81:
			case 0x91:
			case 0xa1:
			case 0xb1:
			case 0xc1:
			case 0xd1:
			case 0xe1:
			case 0xf1:
				this->dp = 0xffde - (((opcode_number & 0xF0) >> 4) << 1);
				this->rd = op_readaddr(this->dp);
				this->rd |= op_readaddr(this->dp + 1) << 8;
				this->op_io(3);
				this->op_writestack(this->regs.pc >> 8);
				this->op_writestack(this->regs.pc);
				this->regs.pc = this->rd;
				break;

			case 0x0f:
				this->rd = op_readaddr(0xffde);
				this->rd |= op_readaddr(0xffdf) << 8;
				this->op_io(2);
				this->op_writestack(this->regs.pc >> 8);
				this->op_writestack(this->regs.pc);
				this->op_writestack(this->regs.p);
				this->regs.pc = this->rd;
				this->regs.p.b = 1;
				this->regs.p.i = 0;
				break;

			case 0x6f:
				this->rd = this->op_readstack();
				this->rd |= this->op_readstack() << 8;
				this->op_io(2);
				this->regs.pc = this->rd;
				break;

			case 0x7f:
				this->regs.p = this->op_readstack();
				this->rd = this->op_readstack();
				this->rd |= this->op_readstack() << 8;
				this->op_io(2);
				this->regs.pc = this->rd;
				break;

			// End of core/oppseudo_pc.cpp

			// Start of core/oppseudo_read.cpp

			case 0x88:
				this->rd = op_readpc();
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x28:
				this->rd = op_readpc();
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x68:
				this->rd = op_readpc();
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0xc8:
				this->rd = op_readpc();
				this->regs.x = this->op_cmp(this->regs.x, this->rd);
				break;

			case 0xad:
				this->rd = op_readpc();
				this->regs.B.y = this->op_cmp(this->regs.B.y, this->rd);
				break;

			case 0x48:
				this->rd = op_readpc();
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x08:
				this->rd = op_readpc();
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xa8:
				this->rd = op_readpc();
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x86:
				this->op_io();
				this->rd = op_readdp(this->regs.x);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x26:
				this->op_io();
				this->rd = op_readdp(this->regs.x);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x66:
				this->op_io();
				this->rd = op_readdp(this->regs.x);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x46:
				this->op_io();
				this->rd = op_readdp(this->regs.x);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x06:
				this->op_io();
				this->rd = op_readdp(this->regs.x);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xa6:
				this->op_io();
				this->rd = op_readdp(this->regs.x);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x84:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x24:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x64:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x3e:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->regs.x = this->op_cmp(this->regs.x, this->rd);
				break;

			case 0x7e:
				switch (++this->opcode_cycle)
				{
					case 1:
						this->dp = op_readpc();
						break;
					case 2:
						this->rd = op_readdp(this->dp);
						this->regs.B.y = this->op_cmp(this->regs.B.y, this->rd);
						this->opcode_cycle = 0;
						break;
				}
				break;

			case 0x44:
				this->dp = op_readpc();
				this->rd = op_readdp(dp);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x04:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xa4:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x94:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x34:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x74:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x54:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x14:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xb4:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x85:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x25:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x65:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x1e:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.x = this->op_cmp(this->regs.x, this->rd);
				break;

			case 0x5e:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.y = this->op_cmp(this->regs.B.y, this->rd);
				break;

			case 0x45:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x05:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xa5:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x95:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.x);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x96:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.B.y);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x35:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.x);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x36:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.B.y);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x75:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.x);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x76:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.B.y);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x55:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.x);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x56:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.B.y);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x15:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.x);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0x16:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.B.y);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xb5:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.x);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0xb6:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->op_io();
				this->rd = op_readaddr(this->dp + this->regs.B.y);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x87:
				this->dp = op_readpc() + this->regs.x;
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x27:
				this->dp = op_readpc() + this->regs.x;
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x67:
				this->dp = op_readpc() + this->regs.x;
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x47:
				this->dp = op_readpc() + this->regs.x;
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x07:
				this->dp = op_readpc() + this->regs.x;
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xa7:
				this->dp = op_readpc() + this->regs.x;
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x97:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp + this->regs.B.y);
				this->regs.B.a = this->op_adc(this->regs.B.a, this->rd);
				break;

			case 0x37:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp + this->regs.B.y);
				this->regs.B.a = this->op_and(this->regs.B.a, this->rd);
				break;

			case 0x77:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp + this->regs.B.y);
				this->regs.B.a = this->op_cmp(this->regs.B.a, this->rd);
				break;

			case 0x57:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp + this->regs.B.y);
				this->regs.B.a = this->op_eor(this->regs.B.a, this->rd);
				break;

			case 0x17:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp + this->regs.B.y);
				this->regs.B.a = this->op_or(this->regs.B.a, this->rd);
				break;

			case 0xb7:
				this->dp = op_readpc();
				this->op_io();
				this->sp = op_readdp(this->dp);
				this->sp |= op_readdp(this->dp + 1) << 8;
				this->rd = op_readaddr(this->sp + this->regs.B.y);
				this->regs.B.a = this->op_sbc(this->regs.B.a, this->rd);
				break;

			case 0x99:
				this->op_io();
				this->rd = op_readdp(this->regs.B.y);
				this->wr = op_readdp(this->regs.x);
				this->wr = this->op_adc(this->wr, this->rd);
				op_writedp(this->regs.x, this->wr);
				break;

			case 0x39:
				this->op_io();
				this->rd = op_readdp(this->regs.B.y);
				this->wr = op_readdp(this->regs.x);
				this->wr = this->op_and(this->wr, this->rd);
				op_writedp(this->regs.x, this->wr);
				break;

			case 0x79:
				this->op_io();
				this->rd = op_readdp(this->regs.B.y);
				this->wr = op_readdp(this->regs.x);
				this->wr = this->op_cmp(wr, rd);
				this->op_io();
				break;

			case 0x59:
				this->op_io();
				this->rd = op_readdp(this->regs.B.y);
				this->wr = op_readdp(this->regs.x);
				this->wr = this->op_eor(this->wr, this->rd);
				op_writedp(this->regs.x, this->wr);
				break;

			case 0x19:
				this->op_io();
				this->rd = op_readdp(this->regs.B.y);
				this->wr = op_readdp(this->regs.x);
				this->wr = this->op_or(this->wr, this->rd);
				op_writedp(this->regs.x, this->wr);
				break;

			case 0xb9:
				this->op_io();
				this->rd = op_readdp(this->regs.B.y);
				this->wr = op_readdp(this->regs.x);
				this->wr = this->op_sbc(this->wr, this->rd);
				op_writedp(this->regs.x, this->wr);
				break;

			case 0x89:
				this->sp = op_readpc();
				this->rd = op_readdp(this->sp);
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_adc(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x29:
				this->sp = op_readpc();
				this->rd = op_readdp(this->sp);
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_and(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x69:
				this->sp = op_readpc();
				this->rd = op_readdp(this->sp);
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_cmp(this->wr, this->rd);
				this->op_io();
				break;

			case 0x49:
				this->sp = op_readpc();
				this->rd = op_readdp(this->sp);
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_eor(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x09:
				this->sp = op_readpc();
				this->rd = op_readdp(this->sp);
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_or(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0xa9:
				this->sp = op_readpc();
				this->rd = op_readdp(this->sp);
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_sbc(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x98:
				this->rd = op_readpc();
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_adc(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x38:
				this->rd = op_readpc();
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_and(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x78:
				this->rd = op_readpc();
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_cmp(this->wr, this->rd);
				this->op_io();
				break;

			case 0x58:
				this->rd = op_readpc();
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_eor(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x18:
				this->rd = op_readpc();
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_or(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0xb8:
				this->rd = op_readpc();
				this->dp = op_readpc();
				this->wr = op_readdp(this->dp);
				this->wr = this->op_sbc(this->wr, this->rd);
				op_writedp(this->dp, this->wr);
				break;

			case 0x7a:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->op_io();
				this->rd |= op_readdp(this->dp + 1) << 8;
				this->regs.ya = this->op_addw(this->regs.ya, this->rd);
				break;

			case 0x9a:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->op_io();
				this->rd |= op_readdp(this->dp + 1) << 8;
				this->regs.ya = this->op_subw(this->regs.ya, this->rd);
				break;

			case 0x5a:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd |= op_readdp(this->dp + 1) << 8;
				this->op_cmpw(this->regs.ya, this->rd);
				break;

			case 0x4a:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->bit = this->dp >> 13;
				this->dp &= 0x1fff;
				this->rd = op_readaddr(this->dp);
				this->regs.p.c = this->regs.p.c & !!(this->rd & (1 << this->bit));
				break;

			case 0x6a:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->bit = this->dp >> 13;
				this->dp &= 0x1fff;
				this->rd = op_readaddr(this->dp);
				this->regs.p.c = this->regs.p.c & !(this->rd & (1 << this->bit));
				break;

			case 0x8a:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->bit = this->dp >> 13;
				this->dp &= 0x1fff;
				this->rd = op_readaddr(this->dp);
				this->op_io();
				this->regs.p.c = this->regs.p.c ^ !!(this->rd & (1 << this->bit));
				break;

			case 0xea:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->bit = this->dp >> 13;
				this->dp &= 0x1fff;
				this->rd = op_readaddr(this->dp);
				this->rd ^= 1 << this->bit;
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x0a:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->bit = this->dp >> 13;
				this->dp &= 0x1fff;
				this->rd = op_readaddr(this->dp);
				this->op_io();
				this->regs.p.c = this->regs.p.c | !!(this->rd & (1 << this->bit));
				break;

			case 0x2a:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->bit = this->dp >> 13;
				this->dp &= 0x1fff;
				this->rd = op_readaddr(this->dp);
				this->op_io();
				this->regs.p.c = this->regs.p.c | !(this->rd & (1 << this->bit));
				break;

			// End of core/oppseudo_read.cpp

			// Start of core/oppseudo_rmw.cpp

			case 0xbc:
				this->op_io();
				this->regs.B.a = this->op_inc(this->regs.B.a);
				break;

			case 0x3d:
				this->op_io();
				this->regs.x = this->op_inc(this->regs.x);
				break;

			case 0xfc:
				this->op_io();
				this->regs.B.y = this->op_inc(this->regs.B.y);
				break;

			case 0x9c:
				this->op_io();
				this->regs.B.a = this->op_dec(this->regs.B.a);
				break;

			case 0x1d:
				this->op_io();
				this->regs.x = this->op_dec(this->regs.x);
				break;

			case 0xdc:
				this->op_io();
				this->regs.B.y = this->op_dec(this->regs.B.y);
				break;

			case 0x1c:
				this->op_io();
				this->regs.B.a = this->op_asl(this->regs.B.a);
				break;

			case 0x5c:
				this->op_io();
				this->regs.B.a = this->op_lsr(this->regs.B.a);
				break;

			case 0x3c:
				this->op_io();
				this->regs.B.a = this->op_rol(this->regs.B.a);
				break;

			case 0x7c:
				this->op_io();
				this->regs.B.a = this->op_ror(this->regs.B.a);
				break;

			case 0xab:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd = this->op_inc(this->rd);
				op_writedp(this->dp, this->rd);
				break;

			case 0x8b:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd = this->op_dec(this->rd);
				op_writedp(this->dp, this->rd);
				break;

			case 0x0b:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd = this->op_asl(this->rd);
				op_writedp(this->dp, this->rd);
				break;

			case 0x4b:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd = this->op_lsr(this->rd);
				op_writedp(this->dp, this->rd);
				break;

			case 0x2b:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd = this->op_rol(this->rd);
				op_writedp(this->dp, this->rd);
				break;

			case 0x6b:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				this->rd = this->op_ror(this->rd);
				op_writedp(this->dp, this->rd);
				break;

			case 0xbb:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->rd = this->op_inc(this->rd);
				op_writedp(this->dp + this->regs.x, this->rd);
				break;

			case 0x9b:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->rd = this->op_dec(this->rd);
				op_writedp(this->dp + this->regs.x, this->rd);
				break;

			case 0x1b:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->rd = this->op_asl(this->rd);
				op_writedp(this->dp + this->regs.x, this->rd);
				break;

			case 0x5b:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->rd = this->op_lsr(this->rd);
				op_writedp(this->dp + this->regs.x, this->rd);
				break;

			case 0x3b:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->rd = this->op_rol(this->rd);
				op_writedp(this->dp + this->regs.x, this->rd);
				break;

			case 0x7b:
				this->dp = op_readpc();
				this->op_io();
				this->rd = op_readdp(this->dp + this->regs.x);
				this->rd = this->op_ror(this->rd);
				op_writedp(this->dp + this->regs.x, this->rd);
				break;

			case 0xac:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->rd = this->op_inc(this->rd);
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x8c:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->rd = this->op_dec(this->rd);
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x0c:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->rd = this->op_asl(this->rd);
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x4c:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->rd = this->op_lsr(this->rd);
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x2c:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->rd = this->op_rol(this->rd);
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x6c:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->rd = this->op_ror(this->rd);
				op_writeaddr(this->dp, this->rd);
				break;

			case 0x0e:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.p.n = !!((this->regs.B.a - this->rd) & 0x80);
				this->regs.p.z = !(this->regs.B.a - this->rd);
				op_readaddr(this->dp);
				op_writeaddr(this->dp, this->rd | this->regs.B.a);
				break;

			case 0x4e:
				this->dp = op_readpc();
				this->dp |= op_readpc() << 8;
				this->rd = op_readaddr(this->dp);
				this->regs.p.n = !!((this->regs.B.a - this->rd) & 0x80);
				this->regs.p.z = !(this->regs.B.a - this->rd);
				op_readaddr(this->dp);
				op_writeaddr(this->dp, this->rd & ~this->regs.B.a);
				break;

			case 0x3a:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				++this->rd;
				op_writedp(this->dp++, this->rd);
				this->rd += op_readdp(this->dp) << 8;
				op_writedp(this->dp, this->rd >> 8);
				this->regs.p.n = !!(this->rd & 0x8000);
				this->regs.p.z = !this->rd;
				break;

			case 0x1a:
				this->dp = op_readpc();
				this->rd = op_readdp(this->dp);
				--this->rd;
				op_writedp(this->dp++, this->rd);
				this->rd += op_readdp(this->dp) << 8;
				op_writedp(this->dp, this->rd >> 8);
				this->regs.p.n = !!(this->rd & 0x8000);
				this->regs.p.z = !this->rd;
				break;

			// End of core/oppseudo_rmw.cpp
		}
	}
}
