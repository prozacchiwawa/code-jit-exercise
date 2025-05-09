#ifndef CONSTANTS_H
#define CONSTANTS_H

constexpr int TLB_SIZE = 4;
constexpr int TRANSLATION_SIZE = 16 + 3 * 8 * 2048;
constexpr int TARGET_INSTR_SHIFT = 1;
constexpr int HOST_INSTR_SHIFT = 3;
constexpr int PAGE_MASK = 0xfff;
constexpr int NEXT_ADDR = 16384;
constexpr int TRANSL_ADDR = 32768;

constexpr int CODE_TR = 0;
constexpr int NEXT_TR = 1;
constexpr int OVERFLOW_TR = 2;

#endif//CONSTANTS_H
