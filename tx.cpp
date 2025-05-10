#include <stdio.h>
#include <stdint.h>
#include "constants.h"
#include "translation.h"
#include "instructions.h"
#include "memory.h"

int main(int argc, char **argv) {
  initialize_translate_code();
  uint8_t bytes[] = {
    0xd0, (2 << 3), // add instruction
    0x66, 0xfe, // beq -2
    0x4e, 0x41, // trap 1
  };

  for (auto i = 0; i < sizeof(bytes); i++) {
    write_byte(0x1000 + i, bytes[i]);
  }

  // Put a value in memory.
  write_byte(0x2000, 0xff);

  cpu_t cpu;
  cpu.d_regs[0] = 1;
  cpu.a_regs[0] = 0x2000;

  cpu.run_code(&cpu, 0x1000);
}
