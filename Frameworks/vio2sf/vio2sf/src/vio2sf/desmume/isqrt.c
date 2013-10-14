#include "isqrt.h"

uint32_t isqrt32(uint32_t n) {
  uint32_t s, t;

#define sqrtBit(k) \
  t = s+(1U<<(k-1)); t <<= k+1; if (n >= t) { n -= t; s |= 1U<<k; }

  s = 0U;
  if (n >= 1U<<30) { n -= 1U<<30; s = 1U<<15; }
  sqrtBit(14); sqrtBit(13); sqrtBit(12); sqrtBit(11); sqrtBit(10);
  sqrtBit(9); sqrtBit(8); sqrtBit(7); sqrtBit(6); sqrtBit(5);
  sqrtBit(4); sqrtBit(3); sqrtBit(2); sqrtBit(1);
  if (n > s<<1) s |= 1U;

#undef sqrtBit

  return s;
}

uint64_t isqrt64(uint64_t n) {
  uint64_t s, t;

#define sqrtBit(k) \
  t = s+(1ULL<<(k-1)); t <<= k+1; if (n >= t) { n -= t; s |= 1ULL<<k; }

  s = 0ULL;
  if (n >= 1ULL<<62) { n -= 1ULL<<62; s = 1ULL<<31; }
  sqrtBit(30); sqrtBit(29); sqrtBit(28); sqrtBit(27); sqrtBit(26);
  sqrtBit(25); sqrtBit(24); sqrtBit(23); sqrtBit(22); sqrtBit(21);
  sqrtBit(20); sqrtBit(19); sqrtBit(18); sqrtBit(17); sqrtBit(16);
  sqrtBit(15);
  sqrtBit(14); sqrtBit(13); sqrtBit(12); sqrtBit(11); sqrtBit(10);
  sqrtBit(9); sqrtBit(8); sqrtBit(7); sqrtBit(6); sqrtBit(5);
  sqrtBit(4); sqrtBit(3); sqrtBit(2); sqrtBit(1);
  if (n > s<<1) s |= 1ULL;

#undef sqrtBit

  return s;
}
