#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include "constants.h"

uint8_t *allocate_executable_code(size_t size);

uint8_t *get_overflow_code_page(uint32_t code);

class overflow_code_t {
 public:
	uint32_t offset;
	uint32_t size;
	uint8_t *translated_code;

 overflow_code_t() : offset(0), size(TRANSLATION_SIZE), translated_code(nullptr) {
		this->translated_code = allocate_executable_code(TRANSLATION_SIZE);
		if (this->translated_code == MAP_FAILED) {
			abort();
		}
	}
};

#endif//UTIL_H
