// Namco 106 sound chip emulator

// Nes_Snd_Emu 0.1.8
#ifndef NES_NAMCO_APU_H
#define NES_NAMCO_APU_H

#include "blargg_common.h"
#include "Blip_Buffer.h"

struct namco_state_t;

class Nes_Namco_Apu {
public:
	// See Nes_Apu.h for reference.
	void volume( double );
	void treble_eq( const blip_eq_t& );
	void output( Blip_Buffer* );
	static const int osc_count = 8;
	void osc_output( int index, Blip_Buffer* );
	void reset();
	void end_frame( blip_time_t );

	// Read/write data register is at 0x4800
	static const unsigned int data_reg_addr = 0x4800;
	void write_data( blip_time_t, int );
	int read_data();

	// Write-only address register is at 0xF800
	static const unsigned int addr_reg_addr = 0xF800;
	void write_addr( int );

	// to do: implement save/restore
	void save_state( namco_state_t* out ) const;
	void load_state( namco_state_t const& );

public:
	Nes_Namco_Apu();
	BLARGG_DISABLE_NOTHROW
private:
	// noncopyable
	Nes_Namco_Apu( const Nes_Namco_Apu& );
	Nes_Namco_Apu& operator = ( const Nes_Namco_Apu& );

	struct Namco_Osc {
		int32_t delay;
		Blip_Buffer* output;
		short last_amp;
		short wave_pos;
	};

	Namco_Osc oscs [osc_count];

	blip_time_t last_time;
	int addr_reg;

	static const int reg_count = 0x80;
	uint8_t reg [reg_count];
	Blip_Synth<blip_good_quality,15> synth;

	uint8_t& access();
	void run_until( blip_time_t );
};
/*
struct namco_state_t
{
	uint8_t regs [0x80];
	uint8_t addr;
	uint8_t unused;
	uint8_t positions [8];
	uint32_t delays [8];
};
*/

inline uint8_t& Nes_Namco_Apu::access()
{
	int addr = addr_reg & 0x7F;
	if ( addr_reg & 0x80 )
		addr_reg = (addr + 1) | 0x80;
	return reg [addr];
}

inline void Nes_Namco_Apu::volume( double v ) { synth.volume( 0.10 / osc_count * v ); }

inline void Nes_Namco_Apu::treble_eq( const blip_eq_t& eq ) { synth.treble_eq( eq ); }

inline void Nes_Namco_Apu::write_addr( int v ) { addr_reg = v; }

inline int Nes_Namco_Apu::read_data() { return access(); }

inline void Nes_Namco_Apu::osc_output( int i, Blip_Buffer* buf )
{
	assert( (unsigned) i < osc_count );
	oscs [i].output = buf;
}

inline void Nes_Namco_Apu::write_data( blip_time_t time, int data )
{
	run_until( time );
	access() = data;
}

#endif
