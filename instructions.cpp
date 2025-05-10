#include <memory>
#include "instructions.h"
#include "translation.h"
#include "cpu.h"

uint8_t *use_translate_code;

void translate_insn(cpu_t *cpu, translation_t *translation_page, uint32_t addr) {
  uint16_t source;
  cpu->pc = translation_page->address + addr;
  source = (cpu->read_byte(cpu->pc) << 8) | cpu->read_byte(cpu->pc + 1);

  std::unique_ptr<instr> instr = std::make_unique<no_translation>();
  int8_t branch_target, code;

  switch (source >> 12) {
  case 0x4: // misc
    switch ((source >> 4) & 0xfff) {
    case 0x4e4:
      code = source & 15;
      instr.reset(new trap(code));
      break;

    default:
      abort();
      break;
    }
    break;

  case 0x6: // branch
    switch ((source >> 8) & 0xf) {
    case 7:
      branch_target = (int8_t)source & 0xff;
      instr.reset(new beq_local(branch_target));
      break;

    case 6:
      branch_target = (int8_t)source & 0xff;
      instr.reset(new bne_local(branch_target));
      break;

    default:
      abort();
      break;
    }
    break;

  case 0xd: // add
    switch ((source >> 6) & 7) {
    case 0:
      switch ((source >> 3) & 7) {
      case 2:
        // dn = (am) + dn
        instr.reset(new add_dn_eq_dn_plus_ea_byte((source >> 9) & 7, source & 7));
        break;

      default:
        abort();
        break;
      }
      break;

    default:
      abort();
      break;
    }
    break;

  default:
    abort();
    break;
  }

  instr->write(translation_page, addr & 0xfff);
}

// The translate instruction works like this:
//
// We start with
// - primary code = indirect call
// - overflow points to proxy to call translate_insn
// - next = 12 bits of pc
//
// The translation code
//   - synthesizes pc
//   - subtracts 8 * ((pc & 0xfff) / 2)
void initialize_translate_code() {
  uint8_t bytes[] = {
    /*9e:*/	0x5a, //                   	pop    %rdx
    /*a2:*/	0x48, 0x83, 0xea, 0x06, //          	sub    $0x6,%rdx
    /*9f:*/	0x49, 0x89, 0xd0, //             	mov    %rdx,%r8
    /*a3:*/	0x4c, 0x8b, 0xa2, 0x00, 0x40, 0x00, 0x00, // 	mov    0x4000(%rdx),%r12
    /*ad:*/	0x4d, 0x89, 0xe5, //             	mov    %r12,%r13
    /*b1:*/	0x49, 0xc1, 0xe4, 0x02, //          	shl    $0x2,%r12
    /*b5:*/	0x49, 0xf7, 0xdc, //             	neg    %r12
    /*b1:*/	0x49, 0x83, 0xec, 0x10, //          	sub    $0x10,%r12
    /*b5:*/	0x49, 0x01, 0xd4, //             	add    %rdx,%r12
    /*b8:*/	0x4c, 0x89, 0xe3, //             	mov    %r12,%rbx
    /*c3:*/	0x4c, 0x8b, 0x0b, //             	mov    (%rbx),%r9
    /*c6:*/	0x4c, 0x8b, 0x5b, 0x08, //          	mov    0x8(%rbx),%r11
    /*ca:*/	0x57, //                   	push   %rdi
    /*cb:*/	0x56, //                   	push   %rsi
    /*cc:*/	0x52, //                   	push   %rdx
    /*cb:*/	0x41, 0x50, //                	push   %r8
    /*cd:*/	0x4c, 0x89, 0xcf, //             	mov    %r9,%rdi
    /*d0:*/	0x4c, 0x89, 0xde, //             	mov    %r11,%rsi
    /*d1:*/	0x4c, 0x89, 0xea, //             	mov    %r13,%rdx
    /*ce:*/	0x41, 0xff, 0x51, 0x50, //          	call   *0x18(%r9)
    /*da:*/	0x41, 0x58, //                	pop    %r8
    /*d8:*/	0x5a, //                   	pop    %rdx
    /*d9:*/	0x5e, //                   	pop    %rsi
    /*da:*/	0x5f, //                   	pop    %rdi
    /*db:*/	0x41, 0xff, 0xe0, //             	jmp    *%r8
  };

  if (!use_translate_code) {
    use_translate_code = get_overflow_code_page(sizeof(bytes));
    memcpy(use_translate_code, bytes, sizeof(bytes));
  }
}

void translate::write(translation_t *translation_page, uint32_t target_on_page) const {
    uint8_t bytes[] = {
      /*89:*/	0xff, 0x15, 0x00, 0x00, 0x00, 0x00, //    	call    *0x0(%rip)        # 8f <indirect_instr+0x6>
    };

    // Use the translated address to target code to call the translation function.
    auto target = translation_page->data_for_translation(CODE_TR, target_on_page);
    memcpy(target, bytes, sizeof(bytes));
    auto corrected_transl = TRANSL_ADDR - 6;
    target[2] = (uint8_t)corrected_transl;
    target[3] = corrected_transl >> 8;
    target[4] = corrected_transl >> 16;
    target[5] = corrected_transl >> 24;

    // Write the translation stub.
    auto transl_addr_target = translation_page->data_for_translation(OVERFLOW_TR, target_on_page);
    memcpy(transl_addr_target, (uint8_t *)&use_translate_code, sizeof(use_translate_code));

    uint64_t page_target_ptr_val = target_on_page;
    auto next_addr_target = translation_page->data_for_translation(NEXT_TR, target_on_page);
    memcpy(next_addr_target, &page_target_ptr_val, sizeof(page_target_ptr_val));
}

void add_dn_eq_dn_plus_ea_byte::write(translation_t *translation, uint32_t page_addr) const {
 {
    uint8_t bytes[] = {
      /*0:*/	0x58, //                   	pop    %rax
      /*1:*/	0x48, 0x83, 0xe8, 0x06, //         	sub    $0x6,%rax
      /*5:*/	0x4c, 0x8b, 0x80, 0x00, 0x40, 0x00, 0x00, //	mov    0x4000(%rax),%r8
      /*c:*/	0x48, 0x89, 0xfb, //             	mov    %rdi,%rbx
      /*f:*/	0x57, //                   	push   %rdi
      /*10:*/	0x8b, 0x7b, 0x21, //            	mov    0x21(%rbx),%edi
      /*13:*/	0xff, 0x53, 0x48, //            	call   *0x48(%rbx)
      /*16:*/	0x5f, //                   	pop    %rdi
      /*17:*/	0x8b, 0x53, 0x4d, //            	mov    0x4d(%rbx),%edx
      /*1a:*/	0x83, 0x4b, 0x44, 0x04, //          	orl    $0x4,0x44(%rbx)
      /*1e:*/	0x01, 0xd0, //                	add    %edx,%eax
      /*20:*/	0x89, 0x43, 0x4d, //             	mov    %eax,0x4d(%rbx)
      /*22:*/	0x74, 0x04, //                	je     26 <end_add_dn_eq_dn_plus_ea_byte>
      /*24:*/	0x83, 0x73, 0x44, 0x04, //          	xorl   $0x4,0x44(%rbx)
      /*28:*/	0x41, 0xff, 0xe0, //            	jmp    *%r8
    };

    auto target = get_overflow_code_page(sizeof(bytes));
    memcpy(target, bytes, sizeof(bytes));
    target[0x12] = 32 + (an * 4);
    target[0x19] = dn * 4;
    target[0x22] = dn * 4;

    memcpy(translation->data_for_translation(OVERFLOW_TR, page_addr), (uint8_t *)&target, sizeof(target));

    auto return_target = translation->data_for_translation(CODE_TR, page_addr + 2);
    auto return_ptr = translation->data_for_translation(NEXT_TR, page_addr);
    memcpy(return_ptr, &return_target, sizeof(return_target));
  }
}

void beq_local::write(translation_t *translation, uint32_t page_addr) const {
  uint8_t bytes[] = {
    /*2f:*/	0x58, //                   	pop    %rax
    /*30:*/	0x48, 0x83, 0xe8, 0x06, //         	sub    $0x6,%rax
    /*34:*/	0xf7, 0x47, 0x44, 0x04, 0x00, 0x00, 0x00, //	testl  $0x4,0x44(%rdi)
    /*3c:*/	0x74, 0x06, //               	je     44 <dont_branch_beq_local>
    /*3e:*/	0xff, 0xa0, 0x00, 0x40, 0x00, 0x00, //   	jmp    *0x7ffa(%rax)
    /*3d:*/	0x48, 0x83, 0xc0, 0x08, //         	add    $0x8,%rax
    /*41:*/	0xff, 0xe0, //               	jmp    *%rax
  };

  auto target = get_overflow_code_page(sizeof(bytes));
  memcpy(target, bytes, sizeof(bytes));

  memcpy(translation->data_for_translation(OVERFLOW_TR, page_addr), (uint8_t *)&target, sizeof(target));

  auto return_target = translation->data_for_translation(CODE_TR, page_addr + relative_target);
  auto return_ptr = translation->data_for_translation(NEXT_TR, page_addr);
  memcpy(return_ptr, &return_target, sizeof(return_target));
}

void bne_local::write(translation_t *translation, uint32_t page_addr) const {
  uint8_t bytes[] = {
    /*2f:*/	0x58, //                   	pop    %rax
    /*30:*/	0x48, 0x83, 0xe8, 0x06, //         	sub    $0x6,%rax
    /*34:*/	0xf7, 0x47, 0x44, 0x04, 0x00, 0x00, 0x00, //	testl  $0x4,0x44(%rdi)
    /*3c:*/	0x74, 0x06, //               	je     44 <dont_branch_beq_local>
    /*3d:*/	0x48, 0x83, 0xc0, 0x08, //         	add    $0x8,%rax
    /*41:*/	0xff, 0xe0, //               	jmp    *%rax
    /*3e:*/	0xff, 0xa0, 0x00, 0x40, 0x00, 0x00, //   	jmp    *0x7ffa(%rax)
  };

  auto target = get_overflow_code_page(sizeof(bytes));
  memcpy(target, bytes, sizeof(bytes));

  memcpy(translation->data_for_translation(OVERFLOW_TR, page_addr), (uint8_t *)&target, sizeof(target));

  auto return_target = translation->data_for_translation(CODE_TR, page_addr + relative_target);
  auto return_ptr = translation->data_for_translation(NEXT_TR, page_addr);
  memcpy(return_ptr, &return_target, sizeof(return_target));
}

void trap::write(translation_t *translation, uint32_t page_addr) const {
  auto target = translation->data_for_translation(CODE_TR, page_addr);
  target[0] = 0xc3;
}

void indirect::write(translation_t *translation, uint32_t page_addr) const {
  uint8_t bytes[] = {
    /*89:*/	0xff, 0x25, 0x00, 0x00, 0x00, 0x00 //    	jmp    *0x0(%rip)        # 8f <indirect_instr+0x6>
  };
  auto target = translation->data_for_translation(CODE_TR, page_addr);
  memcpy(target, bytes, sizeof(bytes));
  auto corrected_transl = TRANSL_ADDR - 6;
  target[2] = corrected_transl;
  target[3] = corrected_transl >> 8;
  target[4] = corrected_transl >> 16;
  target[5] = corrected_transl >> 24;
}
