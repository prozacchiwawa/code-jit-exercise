#include <map>
#include "cpu.h"
#include "translation.h"

std::map<uint32_t, translation_t*> translations;

translation_t *get_translation(cpu_t *cpu, uint32_t page_addr) {
  auto page = page_addr & ~0xfff;
  auto found = translations.find(page);
  translation_t *t;
  if (found == translations.end()) {
    t = new translation_t(cpu, page_addr);
    translations.insert(std::make_pair(page, t));
  } else {
    t = found->second;
  }
  return t;
}

translation_t::translation_t(cpu_t *cpu, uint32_t page_addr) : code_and_targets(nullptr), address(page_addr) {
  this->code_and_targets = allocate_executable_code(TRANSLATION_SIZE);
  if (this->code_and_targets == MAP_FAILED) {
    abort();
  }

  // The beginning of the range contains the cpu and self pointers.
  memcpy(this->code_and_targets, &cpu, sizeof(cpu));
  auto this_ptr = this;
  memcpy(this->code_and_targets + 8, &this_ptr, sizeof(this_ptr));

  // Fill the page with calls to translate.
  auto translate_insn = translate();
  for (auto i = 0; i < 4096 >> TARGET_INSTR_SHIFT; i++) {
    auto in_page_addr = i << TARGET_INSTR_SHIFT;
    translate_insn.write(this, in_page_addr);
  }
}
