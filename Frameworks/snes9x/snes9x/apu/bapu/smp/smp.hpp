#pragma once

#include <memory>
#include <snes9x/snes.hpp>

struct S9xSPC;

namespace SNES
{
	class SMP : public Processor
	{
	public:
		static const uint8_t iplrom[64];
		std::unique_ptr<uint8_t[]> apuram;

		unsigned port_read(unsigned port);
		void port_write(unsigned port, unsigned data);

		unsigned mmio_read(unsigned addr);
		void mmio_write(unsigned addr, unsigned data);

		void enter();
		void power(struct S9xSPC *SPC);
		void reset();

		SMP();
		~SMP();

	//private:
		struct Flags
		{
			bool n, v, p, b, h, i, z, c;

			operator unsigned() const
			{
				return (this->n << 7) | (this->v << 6) | (this->p << 5) | (this->b << 4) | (this->h << 3) | (this->i << 2) | (this->z << 1) | (this->c << 0);
			}

			unsigned operator=(unsigned data)
			{
				this->n = !!(data & 0x80);
				this->v = !!(data & 0x40);
				this->p = !!(data & 0x20);
				this->b = !!(data & 0x10);
				this->h = !!(data & 0x08);
				this->i = !!(data & 0x04);
				this->z = !!(data & 0x02);
				this->c = !!(data & 0x01);
				return data;
			}

			unsigned operator|=(unsigned data)
			{
				return this->operator=(this->operator unsigned() | data);
			}

			unsigned operator^=(unsigned data)
			{
				return this->operator=(this->operator unsigned() ^ data);
			}

			unsigned operator&=(unsigned data)
			{
				return this->operator=(this->operator unsigned() & data);
			}
		};

		unsigned opcode_number;
		unsigned opcode_cycle;

		uint16_t rd, wr, dp, sp, ya, bit;

		struct Regs
		{
			uint16_t pc;
			uint8_t sp;
			union
			{
				uint16_t ya;
#ifdef __BIG_ENDIAN__
				struct { uint8_t y, a; } B;
#else
				struct { uint8_t a, y; } B;
#endif
			};
			uint8_t x;
			Flags p;
		} regs;

		struct Status
		{
			// $00f1
			bool iplrom_enable;

			// $00f2
			unsigned dsp_addr;

			// $00f8,$00f9
			unsigned ram00f8;
			unsigned ram00f9;
		} status;

		template<unsigned frequency> struct Timer
		{
			bool enable;
			uint8_t target;
			uint8_t stage1_ticks;
			uint8_t stage2_ticks;
			uint8_t stage3_ticks;

			void tick();
			void tick(unsigned clocks);
		};

		Timer<128> timer0;
		Timer<128> timer1;
		Timer<16> timer2;

		S9xSPC *SPC;

		inline void tick();
		inline void tick(unsigned clocks);
		inline void op_io();
		inline void op_io(unsigned clocks);
		inline uint8_t op_read(uint16_t addr);
		inline void op_write(uint16_t addr, uint8_t data);
		void op_step();
		void op_writestack(uint8_t data);
		uint8_t op_readstack();
		uint64_t cycle_table_cpu[256];
		unsigned cycle_table_dsp[256];
		uint64_t cycle_step_cpu;

		uint8_t op_adc(uint8_t x, uint8_t y);
		uint16_t op_addw(uint16_t x, uint16_t y);
		uint8_t op_and(uint8_t x, uint8_t y);
		uint8_t op_cmp(uint8_t x, uint8_t y);
		uint16_t op_cmpw(uint16_t x, uint16_t y);
		uint8_t op_eor(uint8_t x, uint8_t y);
		uint8_t op_inc(uint8_t x);
		uint8_t op_dec(uint8_t x);
		uint8_t op_or(uint8_t x, uint8_t y);
		uint8_t op_sbc(uint8_t x, uint8_t y);
		uint16_t op_subw(uint16_t x, uint16_t y);
		uint8_t op_asl(uint8_t x);
		uint8_t op_lsr(uint8_t x);
		uint8_t op_rol(uint8_t x);
		uint8_t op_ror(uint8_t x);
	};
}
