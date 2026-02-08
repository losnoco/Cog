#include <snes9x/snes9x.h>

#include <snes9x/snes.hpp>
#include <snes9x/sdsp.hpp>

#include "../snes/snes.hpp"
#include "sdsp.hpp"
#include "SPC_DSP.h"
#include "../smp/smp.hpp"

namespace SNES
{
	void DSP::power(struct S9xState *st)
	{
		this->spc_dsp.init(&st->Settings, st->SPC.smp->apuram.get());
		this->spc_dsp.reset();
		this->clock = 0;
	}

	void DSP::reset()
	{
		this->spc_dsp.soft_reset();
		this->clock = 0;
	}

	DSP::DSP()
	{
		this->clock = 0;
	}
}
