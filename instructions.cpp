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
  int8_t branch_target;

  switch (source >> 12) {
  case 0x6: // branch
    switch (source & 0xf00) {
    case 7:
      branch_target = (int8_t)source & 0xff;
      instr.reset(new beq_local(branch_target));
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
// - next = low 52 bits of rip, 12 bits of pc.
//
// The translation code
//   - synthesizes pc
//   - subtracts 8 * ((pc & 0xfff) / 2)
void initialize_translate_code() {
  uint8_t bytes[] = {
    /*9e:*/	0x5a, //                   	pop    %rdx
    /*9f:*/	0x49, 0x89, 0xd0, //             	mov    %rdx,%r8
    /*a2:*/	0x48, 0x83, 0xea, 0x06, //          	sub    $0x6,%rdx
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
    /*cd:*/	0x4c, 0x89, 0xcf, //             	mov    %r9,%rdi
    /*d0:*/	0x4c, 0x89, 0xde, //             	mov    %r11,%rsi
    /*d1:*/	0x4c, 0x89, 0xea, //             	mov    %r13,%rdx
    /*ce:*/	0x41, 0xff, 0x51, 0x50, //          	call   *0x18(%r9)
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
      /*0:*/	0x4c, 0x89, 0x44, 0x24, 0x08, //       	mov    %r8,0x8(%rsp)
      /*5:*/	0x48, 0x89, 0xdf, //             	mov    %rbx,%rdi
      /*8:*/	0x8b, 0x73, 0x40, //             	mov    0x40(%rbx),%esi
      /*b:*/	0x8b, 0x7b, 0x44, //             	mov    0x44(%rbx),%edi
      /*e:*/	0xff, 0x51, 0x48, //             	call   *0x48(%rcx)
      /*11:*/	0x01, 0xf0, //                	add    %esi,%eax
      /*13:*/	0x8b, 0x43, 0x40, //             	mov    0x40(%rbx),%eax
      /*16:*/	0x74, 0x0a, //                	je     22 <end_add_dn_eq_dn_plus_ea_byte>
      /*18:*/	0x8b, 0x04, 0x25, 0x04, 0x00, 0x00, 0x00, // 	mov    0x4,%eax
      /*1f:*/	0x09, 0x43, 0x58, //             	or     %eax,0x58(%rbx)
      //0000000000000022 <end_add_dn_eq_dn_plus_ea_byte>:
      /*22:*/	0x41, 0xff, 0xe0 //             	jmp    *%r8
    };

    auto target = get_overflow_code_page(sizeof(bytes));
    // Write in 1 byte at +10 (offsetof dn)
    // Write in 1 byte at +13 (offsetof an)
    // Write in 1 byte at +15 (offsetof read_byte)
    // Write in 1 byte at +33 (offsetof flags)
    memcpy(target, bytes, sizeof(bytes));
    target[10] = dn * 4;
    target[13] = 32 + (an * 4);
    target[15] = 68 + 8;
    target[33] = 64;

    memcpy(translation->data_for_translation(OVERFLOW_TR, page_addr), (uint8_t *)&target, sizeof(target));

    auto return_target = translation->data_for_translation(CODE_TR, page_addr + 2);
    auto return_ptr = translation->data_for_translation(NEXT_TR, page_addr);
    memcpy(return_ptr, return_target, sizeof(return_target));
  }
}

void beq_local::write(translation_t *translation, uint32_t page_addr) const {
  uint8_t bytes[] = {
    /*25:*/	0x4c, 0x89, 0x44, 0x24, 0x08, //       	mov    %r8,0x8(%rsp)
    /*2a:*/	0x8b, 0x04, 0x25, 0x04, 0x00, 0x00, 0x00, // 	mov    0x4,%eax
    /*31:*/	0x85, 0x43, 0x58, //             	test   %eax,0x58(%rbx)
    /*34:*/	0x74, 0x0c, //                	je     42 <dont_branch_beq_local>
    /*36:*/	0xa1, 0xdd, 0xcc, 0xbb, 0xaa, 0x00, 0x00, // 	movabs 0xaabbccdd,%eax
    /*3d:*/	0x00, 0x00,
    /*3f:*/	0x49, 0x01, 0xc0, //             	add    %rax,%r8
    //0000000000000042 <dont_branch_beq_local>:
    /*42:*/	0x41, 0xff, 0xe0 //             	jmp    *%r8
  };

  auto target = get_overflow_code_page(sizeof(bytes));
  // Write in 1 byte at 14 (offsetof flags)
  // Write in 4 bytes at 17 (target relative to following instruction)
  memcpy(target, bytes, sizeof(bytes));
  target[14] = 68;
  target[17] = relative_target;
  target[18] = relative_target >> 8;
  target[19] = relative_target >> 16;
  target[20] = relative_target >> 24;
  memcpy(translation->data_for_translation(OVERFLOW_TR, page_addr), (uint8_t *)&target, sizeof(target));
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
