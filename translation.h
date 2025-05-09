#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <sys/mman.h>
#include <stdint.h>
#include "cpu.h"
#include "instructions.h"

class cpu_t;

class translation_t {
public:
  uint8_t *code_and_targets;
  uint32_t address;

	translation_t(cpu_t *cpu, uint32_t address);

  // Get a pointer to part of the translation info relating to the given
  // in-page address.
  //
  // There are 3 parts currently:
  //
  // 1) 8 bytes of x86_64 code
  // 2) 8 bytes of address targeting out-of-line translated code
  // 3) 8 bytes of next-instruction address
  uint8_t *data_for_translation(int part, uint32_t page_target) const {
    return &this->code_and_targets[16 + (8 * part * 2048) + ((page_target >> TARGET_INSTR_SHIFT) << HOST_INSTR_SHIFT)];
  }
};

translation_t *get_translation(cpu_t *cpu, uint32_t page_addr);

#endif//TRANSLATION_H
