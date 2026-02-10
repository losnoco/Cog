#pragma once

#include <cstdint>

namespace SNES
{
	struct Processor
	{
		unsigned frequency;
		int32_t clock;
	};

	class CPU
	{
	public:
		enum { Threaded = false };
		int frequency;
		uint8_t registers[4];

		void reset()
		{
			this->registers[0] = this->registers[1] = this->registers[2] = this->registers[3] = 0;
		}

		void port_write(uint8_t port, uint8_t data)
		{
			this->registers[port & 3] = data;
		}

		uint8_t port_read(uint8_t port)
		{
			return this->registers[port & 3];
		}
	};
}
