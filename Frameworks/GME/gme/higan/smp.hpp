#ifndef _higan_smp_h_
#define _higan_smp_h_

#include "blargg_common.h"

#include "../dsp/dsp.hpp"

namespace SuperFamicom {

struct SMP {
  long clock;

  uint8_t iplrom[64];
  uint8_t* apuram;

  SuperFamicom::DSP dsp;

  inline void synchronize_dsp();

  unsigned port_read(unsigned port);
  void port_write(unsigned port, unsigned data);

  unsigned mmio_read(unsigned addr);
  void mmio_write(unsigned addr, unsigned data);

  void enter();
  void power();
  void reset();
    
  void render(int16_t * buffer, unsigned count);
  void skip(unsigned count);

  uint8_t sfm_last[4];
private:
  uint8_t const* sfm_queue;
  uint8_t const* sfm_queue_end;
public:
  void set_sfm_queue(const uint8_t* queue, const uint8_t* queue_end);

private:
  int16_t * sample_buffer;
  int16_t const* sample_buffer_end;
public:
  void sample( int16_t, int16_t );

  SMP();
  ~SMP();

//private:
  struct Flags {
    bool n, v, p, b, h, i, z, c;

    inline operator unsigned() const {
      return (n << 7) | (v << 6) | (p << 5) | (b << 4)
           | (h << 3) | (i << 2) | (z << 1) | (c << 0);
    };

    inline unsigned operator=(unsigned data) {
      n = data & 0x80; v = data & 0x40; p = data & 0x20; b = data & 0x10;
      h = data & 0x08; i = data & 0x04; z = data & 0x02; c = data & 0x01;
      return data;
    }

    inline unsigned operator|=(unsigned data) { return operator=(operator unsigned() | data); }
    inline unsigned operator^=(unsigned data) { return operator=(operator unsigned() ^ data); }
    inline unsigned operator&=(unsigned data) { return operator=(operator unsigned() & data); }
  };

  unsigned opcode_number;
  unsigned opcode_cycle;

  struct Regs {
    uint16_t pc;
    uint8_t sp;
    union {
      uint16_t ya;
#ifdef BLARGG_BIG_ENDIAN
      struct { uint8_t y, a; };
#else
      struct { uint8_t a, y; };
#endif
    };
    uint8_t x;
    Flags p;
  } regs;

  uint16_t rd, wr, dp, sp, ya, bit;

  struct Status {
    //$00f1
    bool iplrom_enable;

    //$00f2
    unsigned dsp_addr;

    //$00f8,$00f9
    unsigned ram00f8;
    unsigned ram00f9;
  } status;

  template<unsigned frequency>
  struct Timer {
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
  Timer< 16> timer2;

  void tick();
  inline void op_io();
  inline uint8_t op_read(uint16_t addr);
  inline void op_write(uint16_t addr, uint8_t data);
  inline void op_step();
  static const unsigned cycle_count_table[256];

  uint8_t  op_adc (uint8_t  x, uint8_t  y);
  uint16_t op_addw(uint16_t x, uint16_t y);
  uint8_t  op_and (uint8_t  x, uint8_t  y);
  uint8_t  op_cmp (uint8_t  x, uint8_t  y);
  uint16_t op_cmpw(uint16_t x, uint16_t y);
  uint8_t  op_eor (uint8_t  x, uint8_t  y);
  uint8_t  op_inc (uint8_t  x);
  uint8_t  op_dec (uint8_t  x);
  uint8_t  op_or  (uint8_t  x, uint8_t  y);
  uint8_t  op_sbc (uint8_t  x, uint8_t  y);
  uint16_t op_subw(uint16_t x, uint16_t y);
  uint8_t  op_asl (uint8_t  x);
  uint8_t  op_lsr (uint8_t  x);
  uint8_t  op_rol (uint8_t  x);
  uint8_t  op_ror (uint8_t  x);
};

inline void SMP::set_sfm_queue(const uint8_t *queue, const uint8_t *queue_end) { sfm_queue = queue; sfm_queue_end = queue_end; sfm_last[0] = 0; sfm_last[1] = 0; sfm_last[2] = 0; sfm_last[3] = 0; }
};

#endif
