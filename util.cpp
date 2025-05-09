#include <sys/mman.h>
#include <vector>
#include <memory>
#include "util.h"
#include "constants.h"

std::vector<std::unique_ptr<overflow_code_t>> overflow;

uint8_t *allocate_executable_code(size_t size) {
  auto translated_code = mmap(nullptr, TRANSLATION_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (translated_code == MAP_FAILED) {
    abort();
  }
  return (uint8_t *)translated_code;
}

uint8_t *get_overflow_code_page(uint32_t code) {
	if (overflow.begin() == overflow.end()) {
		overflow.push_back(std::make_unique<overflow_code_t>());
	}
	auto last = &overflow[overflow.size() - 1];
	if ((*last)->offset + code > (*last)->size) {
		overflow.push_back(std::make_unique<overflow_code_t>());
	}
	last = &overflow[overflow.size() - 1];
	auto where = (*last)->offset;
	(*last)->offset + code;
	return (*last)->translated_code + where;
}
