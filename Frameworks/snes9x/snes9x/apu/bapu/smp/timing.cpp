#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

namespace SNES
{
	template<unsigned cycle_frequency> void SMP::Timer<cycle_frequency>::tick()
	{
		if (++this->stage1_ticks < cycle_frequency)
			return;

		this->stage1_ticks = 0;
		if (!this->enable)
			return;

		if (++this->stage2_ticks != this->target)
			return;

		this->stage2_ticks = 0;
		this->stage3_ticks = (this->stage3_ticks + 1) & 15;
	}

	template<unsigned cycle_frequency> void SMP::Timer<cycle_frequency>::tick(unsigned clocks)
	{
		this->stage1_ticks += clocks;
		if (this->stage1_ticks < cycle_frequency)
			return;

		this->stage1_ticks -= cycle_frequency;
		if (!this->enable)
			return;

		if (++this->stage2_ticks != this->target)
			return;

		this->stage2_ticks = 0;
		this->stage3_ticks = (this->stage3_ticks + 1) & 15;
	}

	template void SMP::Timer<128>::tick();
	template void SMP::Timer<128>::tick(unsigned);
	template void SMP::Timer<16>::tick();
	template void SMP::Timer<16>::tick(unsigned);
}
