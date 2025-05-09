# JIT Exercise

This is an exercise to make an extremely tiny jit for an emulator with focus on communicating a few concepts:

- gxemul style lazy translation, where instructions are pre-initialized to self-translate and restart.
- gxemul style "low_pc" system where we think about a page offset in target space as translating to a corresponding target in host translation space with a different scale per instruction.
- leveraging pc relative addressing to set up parallel pointer lists that can augment the instructions to make something like gxemul's DYNTRANS_IC struct, but spread out.
- calling host code from inside the instruction stream using a cpu * that's always in rdi (read_byte in the translated add instruction).  this is needed because memory traffic can be side effecting at the emulation level.

Also it was a fun distraction for a day or two
