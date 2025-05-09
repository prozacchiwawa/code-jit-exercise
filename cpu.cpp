#include "cpu.h"
#include "memory.h"
#include "translation.h"
#include "instructions.h"

cpu_t::cpu_t() {
  memset(d_regs, 0, sizeof(d_regs));
  memset(a_regs, 0, sizeof(a_regs));
  flags = 0;
  pc = 0;
  this->read_byte = ::read_byte;
  this->translate_insn = ::translate_insn;
}

void cpu_t::run_code(struct cpu_t *cpu, uint32_t addr) {
  auto translation = get_translation(cpu, addr);
  run_with_cpu target = (run_with_cpu)translation->data_for_translation(CODE_TR, addr & 0xfff);
  (*target)(cpu);
}
