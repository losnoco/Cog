
#include "Nsf_Emu.h"

#if !NSF_EMU_APU_ONLY
	#include "Nes_Namco_Apu.h"
	#include "Nes_Fds_Apu.h"
	#include "Nes_Mmc5_Apu.h"
#endif

#include "blargg_source.h"

#include <cstring>

int Nsf_Emu::cpu_read( nes_addr_t addr )
{
	int result, i;
	
	result = cpu::low_mem [addr & 0x7FF];
	if ( !(addr & 0xE000) )
		goto exit;

	result = *cpu::get_code( addr );
	if ( addr > 0x7FFF )
		goto exit;

	result = sram [addr & (sizeof sram - 1)];
	if ( addr > 0x5FFF )
		goto exit;

	if ( addr == Nes_Apu::status_addr )
		return apu.read_status( cpu::time() );

	#if !NSF_EMU_APU_ONLY
		if ( addr == Nes_Namco_Apu::data_reg_addr && namco )
			return namco->read_data();
		
		if ( (unsigned) (addr - Nes_Fds_Apu::io_addr) < Nes_Fds_Apu::io_size && fds )
			return fds->read( time(), addr );
		
		i = addr - 0x5C00;
		if ( (unsigned) i < mmc5->exram_size && mmc5 )
			return mmc5->exram [i];
		
		i = addr - 0x5205;
		if ( (unsigned) i < 2 && mmc5 )
			return ((mmc5_mul [0] * mmc5_mul [1]) >> (i * 8)) & 0xFF;
	#endif

	result = addr >> 8; // simulate open bus

	if ( addr != 0x2002 )
		debug_printf( "Read unmapped $%.4X\n", (unsigned) addr );

exit:
	return result;
}

void Nsf_Emu::cpu_write( nes_addr_t addr, int data )
{
	{
		nes_addr_t offset = addr ^ sram_addr;
		if ( offset < sizeof sram )
		{
			sram [offset] = data;
			return;
		}
	}
	{
		int temp = addr & 0x7FF;
		if ( !(addr & 0xE000) )
		{
			cpu::low_mem [temp] = data;
			return;
		}
	}

	if ( unsigned (addr - Nes_Apu::start_addr) <= Nes_Apu::end_addr - Nes_Apu::start_addr )
	{
		GME_APU_HOOK( this, addr - Nes_Apu::start_addr, data );
		apu.write_register( cpu::time(), addr, data );
		return;
	}

	if ( fds && ( addr >= 0x5FF6 && addr <= 0x5FFD ) )
	{
		int32_t offset = rom.mask_addr( data * (int32_t) bank_size );
		if ( offset >= rom.size() )
			set_warning( "Invalid bank" );
		unsigned bank = addr - 0x5FF6;
		byte* out = sram;
		if ( bank >= 2 )
		{
			out = fds->sram;
			bank -= 2;
		}
		memcpy( &out [bank * bank_size], rom.at_addr( offset ), bank_size );
		return;
	}

	unsigned bank = addr - bank_select_addr;
	if ( bank < bank_count )
	{
		int32_t offset = rom.mask_addr( data * (int32_t) bank_size );
		if ( offset >= rom.size() )
			set_warning( "Invalid bank" );
		cpu::map_code( (bank + 8) * bank_size, bank_size, rom.at_addr( offset ) );
		return;
	}

	cpu_write_misc( addr, data );
}

#define CPU_READ( cpu, addr, time )         STATIC_CAST(Nsf_Emu&,*cpu).cpu_read( addr )
#define CPU_WRITE( cpu, addr, data, time )  STATIC_CAST(Nsf_Emu&,*cpu).cpu_write( addr, data )
