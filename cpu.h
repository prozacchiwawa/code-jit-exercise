#ifndef CPU_H
#define CPU_H

#include "translation.h"

class translation_t;

class cpu_t {
 public:
  uint32_t d_regs[8];
  uint32_t a_regs[8];
  uint32_t pc;
  uint32_t flags;

  uint32_t (*read_byte)(uint32_t addr);
  void (*translate_insn)(cpu_t *cpu, translation_t *translation_page, uint32_t addr);

  cpu_t();
  void run_code(cpu_t *cpu, uint32_t addr);
};

typedef void (*run_with_cpu)(cpu_t *cpu);

#endif//CPU_H
