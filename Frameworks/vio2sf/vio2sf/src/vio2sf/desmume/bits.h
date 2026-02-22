#pragma once

inline uint32_t BIT(uint32_t n) { return 1 << n; }

inline uint32_t BIT_N(uint32_t i, uint32_t n) { return (i >> n) & 1; }
inline uint32_t BIT0(uint32_t i) { return i & 1; }
inline uint32_t BIT1(uint32_t i) { return BIT_N(i, 1); }
inline uint32_t BIT2(uint32_t i) { return BIT_N(i, 2); }
inline uint32_t BIT3(uint32_t i) { return BIT_N(i, 3); }
inline uint32_t BIT4(uint32_t i) { return BIT_N(i, 4); }
inline uint32_t BIT5(uint32_t i) { return BIT_N(i, 5); }
inline uint32_t BIT6(uint32_t i) { return BIT_N(i, 6); }
inline uint32_t BIT7(uint32_t i) { return BIT_N(i, 7); }
inline uint32_t BIT8(uint32_t i) { return BIT_N(i, 8); }
inline uint32_t BIT9(uint32_t i) { return BIT_N(i, 9); }
inline uint32_t BIT10(uint32_t i) { return BIT_N(i, 10); }
inline uint32_t BIT11(uint32_t i) { return BIT_N(i, 11); }
inline uint32_t BIT12(uint32_t i) { return BIT_N(i, 12); }
inline uint32_t BIT13(uint32_t i) { return BIT_N(i, 13); }
inline uint32_t BIT14(uint32_t i) { return BIT_N(i, 14); }
inline uint32_t BIT15(uint32_t i) { return BIT_N(i, 15); }
inline uint32_t BIT16(uint32_t i) { return BIT_N(i, 16); }
inline uint32_t BIT17(uint32_t i) { return BIT_N(i, 17); }
inline uint32_t BIT18(uint32_t i) { return BIT_N(i, 18); }
inline uint32_t BIT19(uint32_t i) { return BIT_N(i, 19); }
inline uint32_t BIT20(uint32_t i) { return BIT_N(i, 20); }
inline uint32_t BIT21(uint32_t i) { return BIT_N(i, 21); }
inline uint32_t BIT22(uint32_t i) { return BIT_N(i, 22); }
inline uint32_t BIT23(uint32_t i) { return BIT_N(i, 23); }
inline uint32_t BIT24(uint32_t i) { return BIT_N(i, 24); }
inline uint32_t BIT25(uint32_t i) { return BIT_N(i, 25); }
inline uint32_t BIT26(uint32_t i) { return BIT_N(i, 26); }
inline uint32_t BIT27(uint32_t i) { return BIT_N(i, 27); }
inline uint32_t BIT28(uint32_t i) { return BIT_N(i, 28); }
inline uint32_t BIT29(uint32_t i) { return BIT_N(i, 29); }
inline uint32_t BIT30(uint32_t i) { return BIT_N(i, 30); }
inline uint32_t BIT31(uint32_t i) { return i >> 31; }

inline uint32_t CONDITION(uint32_t i) { return i >> 28; }

inline uint32_t REG_POS(uint32_t i, uint32_t n) { return (i >> n) & 0xF; }
