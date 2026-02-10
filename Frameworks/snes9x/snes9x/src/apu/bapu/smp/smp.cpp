#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

namespace SNES
{
	void SMP::enter()
	{
		while (this->clock < 0)
			this->op_step();
	}

	void SMP::power(struct S9xSPC *SPC)
	{
		this->SPC = SPC;

		Processor::clock = 0;

		this->timer0.target = 0;
		this->timer1.target = 0;
		this->timer2.target = 0;

		this->reset();
	}

	void SMP::reset()
	{
		for (unsigned n = 0x0000; n <= 0xffff; ++n)
			this->apuram[n] = 0x00;

		this->opcode_number = 0;
		this->opcode_cycle = 0;

		this->regs.pc = 0xffc0;
		this->regs.sp = 0xef;
		this->regs.B.a = 0x00;
		this->regs.x = 0x00;
		this->regs.B.y = 0x00;
		this->regs.p = 0x02;

		// $00f1
		this->status.iplrom_enable = true;

		// $00f2
		this->status.dsp_addr = 0x00;

		// $00f8,$00f9
		this->status.ram00f8 = 0x00;
		this->status.ram00f9 = 0x00;

		// timers
		this->timer0.enable = this->timer1.enable = this->timer2.enable = false;
		this->timer0.stage1_ticks = this->timer1.stage1_ticks = this->timer2.stage1_ticks = 0;
		this->timer0.stage2_ticks = this->timer1.stage2_ticks = this->timer2.stage2_ticks = 0;
		this->timer0.stage3_ticks = this->timer1.stage3_ticks = this->timer2.stage3_ticks = 0;
	}

	SMP::SMP()
	{
		this->apuram.reset(new uint8_t[64 * 1024]);
	}

	SMP::~SMP()
	{
	}
}
