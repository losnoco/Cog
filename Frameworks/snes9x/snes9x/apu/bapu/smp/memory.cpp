#include "../../../snes9x.h"

#include "smp.hpp"
#include "../dsp/sdsp.hpp"

namespace SNES
{
	unsigned SMP::port_read(unsigned addr)
	{
		return this->apuram[0xf4 + (addr & 3)];
	}

	void SMP::port_write(unsigned addr, unsigned data)
	{
		this->apuram[0xf4 + (addr & 3)] = data;
	}

	unsigned SMP::mmio_read(unsigned addr)
	{
		switch (addr)
		{
			case 0xf2:
				return this->status.dsp_addr;

			case 0xf3:
				return SPC->dsp->read(this->status.dsp_addr & 0x7f);

			case 0xf4:
			case 0xf5:
			case 0xf6:
			case 0xf7:
				return SPC->cpu->port_read(addr);

			case 0xf8:
				return this->status.ram00f8;

			case 0xf9:
				return this->status.ram00f9;

			case 0xfd:
			{
				unsigned result = this->timer0.stage3_ticks & 15;
				this->timer0.stage3_ticks = 0;
				return result;
			}

			case 0xfe:
			{
				unsigned result = this->timer1.stage3_ticks & 15;
				this->timer1.stage3_ticks = 0;
				return result;
			}

			case 0xff:
			{
				unsigned result = this->timer2.stage3_ticks & 15;
				this->timer2.stage3_ticks = 0;
				return result;
			}
		}

		return 0x00;
	}

	void SMP::mmio_write(unsigned addr, unsigned data)
	{
		switch (addr)
		{
			case 0xf1:
				status.iplrom_enable = !!(data & 0x80);

				if (data & 0x30)
				{
					if (data & 0x20)
					{
						SPC->cpu->port_write(3, 0x00);
						SPC->cpu->port_write(2, 0x00);
					}
					if (data & 0x10)
					{
						SPC->cpu->port_write(1, 0x00);
						SPC->cpu->port_write(0, 0x00);
					}
				}

				if (!this->timer2.enable && !!(data & 0x04))
				{
					this->timer2.stage2_ticks = 0;
					this->timer2.stage3_ticks = 0;
				}
				this->timer2.enable = !!(data & 0x04);

				if (!this->timer1.enable && !!(data & 0x02))
				{
					this->timer1.stage2_ticks = 0;
					this->timer1.stage3_ticks = 0;
				}
				this->timer1.enable = !!(data & 0x02);

				if (!this->timer0.enable && !!(data & 0x01))
				{
					this->timer0.stage2_ticks = 0;
					this->timer0.stage3_ticks = 0;
				}
				this->timer0.enable = !!(data & 0x01);

				break;

			case 0xf2:
				this->status.dsp_addr = data;
				break;

			case 0xf3:
				if (this->status.dsp_addr & 0x80)
					break;
				SPC->dsp->write(this->status.dsp_addr, data);
				break;

			case 0xf4:
			case 0xf5:
			case 0xf6:
			case 0xf7:
				this->port_write(addr, data);
				break;

			case 0xf8:
				this->status.ram00f8 = data;
				break;

			case 0xf9:
				this->status.ram00f9 = data;
				break;

			case 0xfa:
				this->timer0.target = data;
				break;

			case 0xfb:
				this->timer1.target = data;
				break;

			case 0xfc:
				this->timer2.target = data;
				break;
		}
	}
}
