#pragma once

#include <snes9x/snes.hpp>
#include <snes9x/SPC_DSP.h>

struct S9xState;

namespace SNES
{
	class DSP : public Processor
	{
	public:
		uint8_t read(uint8_t addr)
		{
			this->synchronize();
			return this->spc_dsp.read(addr);
		}

		void synchronize()
		{
			if (this->clock)
			{
				this->spc_dsp.run(this->clock);
				this->clock = 0;
			}
		}

		void write(uint8_t addr, uint8_t data)
		{
			this->synchronize();
			this->spc_dsp.write(addr, data);
		}

		void power(struct S9xState *st);
		void reset();

		DSP();

		SPC_DSP spc_dsp;
	};
}
