#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include <string.h>
#include "constants.h"
#include "util.h"
#include "translation.h"

class translation_t;
class cpu_t;

void initialize_translate_code();
void translate_insn(cpu_t *cpu, translation_t *translation_page, uint32_t addr);

class instr {
public:
  virtual bool in_place() const = 0;
  virtual void write(translation_t *page, uint32_t page_addr) const = 0;
};

class trap : public instr {
public:
  int code;

  trap(int code) : code(code) { }
  bool in_place() const { return false; }
  void write(translation_t *page, uint32_t page_addr) const;
};

class no_translation : public instr {
public:
  bool in_place() const { return false; }
  void write(translation_t *target, uint32_t page_addr) const {
    abort();
  }
};

class add_dn_eq_dn_plus_ea_byte : public instr {
public:
  int dn, an;

  add_dn_eq_dn_plus_ea_byte(int dn, int an) : dn(dn), an(an) { }

  bool in_place() const { return false; }
  void write(translation_t *translation, uint32_t page_addr) const;
};

class beq_local : public instr {
public:
  int relative_target;

  beq_local(int relative_target) : relative_target(relative_target) { }

  bool in_place() const { return false; }
  void write(translation_t *translation, uint32_t page_addr) const;
};

class bne_local : public instr {
public:
  int relative_target;

  bne_local(int relative_target) : relative_target(relative_target) { }

  bool in_place() const { return false; }
  void write(translation_t *translation, uint32_t page_addr) const;
};

// Generate in place code for out of line instruction
class indirect : public instr {
public:
  bool in_place() const { return true; }
  void write(translation_t *translation, uint32_t page_addr) const;
};

class translate {
public:
  bool in_place() const { return false; }
  void write(translation_t *translation, uint32_t page_addr) const;
};

#endif//INSTRUCTIONS_H
