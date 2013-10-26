#define CYCLE_ACCURATE

#include "smp.hpp"

#define SMP_CPP
namespace SuperFamicom {

#include "algorithms.cpp"
#include "core.cpp"
#include "memory.cpp"
#include "timing.cpp"

void SMP::synchronize_dsp() {
  while(dsp.clock < 0) dsp.enter();
}

void SMP::enter() {
  while(clock < 0 && sample_buffer < sample_buffer_end) op_step();
}

void SMP::render(int16_t * buffer, unsigned count)
{
  while (count > 4096) {
    sample_buffer = buffer;
    sample_buffer_end = buffer + 4096;
    buffer += 4096;
    count -= 4096;
    clock -= 32 * 24 * 4096;
    enter();
  }
  sample_buffer = buffer;
  sample_buffer_end = buffer + count;
  clock -= 32 * 24 * count;
  enter();
}
        
void SMP::skip(unsigned count)
{
  while (count > 4096) {
    sample_buffer = 0;
    sample_buffer_end = (const int16_t *) 4096;
    count -= 4096;
    clock -= 32 * 24 * 4096;
    enter();
  }
  sample_buffer = 0;
  sample_buffer_end = (const int16_t *) (intptr_t) count;
  clock -= 32 * 24 * count;
  enter();
}
    
void SMP::sample(int16_t left, int16_t right)
{
  if ( sample_buffer >= (const int16_t *) (intptr_t) 4096 ) {
    if ( sample_buffer < sample_buffer_end ) *sample_buffer++ = left;
    if ( sample_buffer < sample_buffer_end ) *sample_buffer++ = right;
  }
  else if ( sample_buffer < sample_buffer_end ){
    sample_buffer += 2;
  }
}

void SMP::power() {
  timer0.target = 0;
  timer1.target = 0;
  timer2.target = 0;

  reset();

  dsp.power();
}

void SMP::reset() {
  for(unsigned n = 0x0000; n <= 0xffff; n++) apuram[n] = 0x00;

  opcode_number = 0;
  opcode_cycle = 0;

  regs.pc = 0xffc0;
  regs.sp = 0xef;
  regs.a = 0x00;
  regs.x = 0x00;
  regs.y = 0x00;
  regs.p = 0x02;

  //$00f1
  status.iplrom_enable = true;

  //$00f2
  status.dsp_addr = 0x00;

  //$00f8,$00f9
  status.ram00f8 = 0x00;
  status.ram00f9 = 0x00;

  //timers
  timer0.enable = timer1.enable = timer2.enable = false;
  timer0.stage1_ticks = timer1.stage1_ticks = timer2.stage1_ticks = 0;
  timer0.stage2_ticks = timer1.stage2_ticks = timer2.stage2_ticks = 0;
  timer0.stage3_ticks = timer1.stage3_ticks = timer2.stage3_ticks = 0;
}

SMP::SMP() : dsp( *this ), clock( 0 ) {
  apuram = new uint8_t[64 * 1024];
  for(auto& byte : iplrom) byte = 0;
  set_sfm_queue(0, 0);
}

SMP::~SMP() {
  delete [] apuram;
}

}
