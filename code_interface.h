#ifndef CODE_INTERFACE_H
#define CODE_INTERFACE_H

#include <stdint.h>

class code_interface_t {
	uint32_t d_regs[8];
	uint32_t a_regs[8];
	uint32_t flags;

	uint8_t *(read_byte)(uint32_t addr);
	uint8_t *(write_byte)(uint32_t addr);
	uint8_t *(trap)(uint32_t dummy);
  uint8_t *(translate)(uint32_t addr);
};

#endif//CODE_INTERFACE_H
