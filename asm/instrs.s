#define NEXT_ADDR 32768
#define TRANSL_ADDR 16384

  .text
  .globl add_dn_eq_dn_plus_ea_byte
add_dn_eq_dn_plus_ea_byte:
  pop %rax
  subq $6, %rax
  movq 16384(%rax), %r8 /* Get the next address */
  movq %rdi, %rbx
  push %rdi
  movl 33(%rbx), %edi /* read the a register */
  callq *0x48(%rbx) /* call read byte */
  pop %rdi
  movl 77(%rbx), %edx /* read d<n> */
  orl $4, 68(%rbx)
  addl %edx, %eax
  jz end_add_dn_eq_dn_plus_ea_byte
  xorl $4, 68(%rbx) /* set zf */
end_add_dn_eq_dn_plus_ea_byte:
  jmpq *%r8

  .globl beq_local
beq_local:
  pop %rax
  subq $6, %rax

  testl $4, 68(%rdi)
  jz dont_branch_beq_local
  jmpq *16384(%rax)
dont_branch_beq_local:
  addq $8, %rax
  jmpq *%rax

  .globl beq
beq:
  pop %rax
  addq 16384, %rax
  movq %rax, %r8

  movl 4, %eax
  testl 88(%rbx), %eax
  jz dont_branch_beq
  movl 0xaabbccdd, %eax
  movl %eax, %edi
  callq *92(%rbx)
  movq %rax, %r8
dont_branch_beq:
  jmpq *%r8

  .globl bra_local
bra_local:
  jmpq *NEXT_ADDR(%rip)

  .globl trap
trap:
  movl 0xaabbccdd, %eax
  movq %rdi, %rbx
  movq %rax, %rdi
  callq *108(%rbx)
  ret

indirect_instr:
  callq *TRANSL_ADDR(%rip)

  .globl translate_insn
translate_insn:
  pop %rdx /* enter with pc on the stack */
  movq %rdx, %r8
  subq $6, %rdx

  /* find the next pointer */
  movq 16384(%rdx), %r12
  movq %r12, %r13
  shlq $2, %r12
  neg %r12
  sub $16, %r12
  add %rdx, %r12

  /* rbx now points to the beginning of this translation block */
  movq %r12, %rbx

  movq (%rbx), %r9 /* pc based offset to the start of the object, containing the cpu pointer */
  movq 8(%rbx), %r11 /* pc based offset to the start of the object, containing a self pointer */
  push %rdi /* set up arguments for translate_insn */
  push %rsi
  push %rdx
  push %r8
  movq %r9, %rdi
  movq %r11, %rsi
  movq %r13, %rdx

  /* call translate */
  callq *0x50(%r9)

  pop %r8
  pop %rdx
  pop %rsi
  pop %rdi

  jmp *%r8
